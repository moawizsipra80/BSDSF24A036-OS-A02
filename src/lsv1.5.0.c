/*
 * Programming Assignment 02: lsv1.5.0
 * Feature: Colorized Output based on File Type
 *
 * Supported options:
 *   -l : Long listing format
 *   -x : Horizontal (across) column display
 *   default : Down-then-across column display
 *
 * Colors:
 *   Blue    : Directory
 *   Green   : Executable
 *   Red     : Tarball/Archive (.tar, .gz, .zip)
 *   Pink    : Symbolic Link
 *   Reverse : Special files (device, socket, fifo)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/ioctl.h>

extern int errno;

#define MAX_FILES 10000
#define SPACING 2

/* =========================================================
 * ANSI Escape Codes for Terminal Colors
 *
 * Format: \033[<code>m
 *   0  = reset
 *   31 = red
 *   32 = green
 *   34 = blue
 *   35 = magenta (pink)
 *   7  = reverse video
 * ========================================================= */
#define COLOR_RESET   "\033[0m"
#define COLOR_BLUE    "\033[0;34m"   /* Directories */
#define COLOR_GREEN   "\033[0;32m"   /* Executables */
#define COLOR_RED     "\033[0;31m"   /* Tarballs/Archives */
#define COLOR_PINK    "\033[0;35m"   /* Symbolic Links */
#define COLOR_REVERSE "\033[0;7m"    /* Special files */

/* Function prototypes */
void do_ls(const char *dir, int long_format, int horizontal_flag);
void list_long(const char *dir, const char *fullpath);
void format_permissions(mode_t mode, char *str);
int compare_strings(const void *a, const void *b);
void display_columns(char **files, int count, const char *dir);
void display_horizontal(char **files, int count, const char *dir);
const char *get_color(const char *filename, struct stat *st);

/* =========================================================
 * MAIN FUNCTION
 * Parses command-line arguments using getopt()
 * ========================================================= */
int main(int argc, char *argv[])
{
    int long_format = 0;
    int horizontal_flag = 0;
    int opt;
    int dir_count = 0;
    char *dirs[100];

    /* Parse options: -l for long, -x for horizontal */
    while ((opt = getopt(argc, argv, "lx")) != -1)
    {
        if (opt == 'l')
            long_format = 1;
        else if (opt == 'x')
            horizontal_flag = 1;
    }

    /* Collect remaining arguments (directories) */
    for (int i = optind; i < argc; i++)
    {
        dirs[dir_count++] = argv[i];
    }

    if (dir_count == 0)
    {
        /* No directory given: list current directory */
        do_ls(".", long_format, horizontal_flag);
    }
    else
    {
        for (int i = 0; i < dir_count; i++)
        {
            if (dir_count > 1)
                printf("%s:\n", dirs[i]);
            do_ls(dirs[i], long_format, horizontal_flag);
            if (i < dir_count - 1)
                puts("");
        }
    }
    return 0;
}

/* =========================================================
 * do_ls()
 * Reads directory contents, sorts them, and displays
 * ========================================================= */
void do_ls(const char *dir, int long_format, int horizontal_flag)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }

    /* Array to store filenames */
    char *files[MAX_FILES];
    int count = 0;

    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        /* Skip hidden files (starting with '.') */
        if (entry->d_name[0] == '.')
            continue;

        if (count >= MAX_FILES)
            break;

        /* Duplicate the name into heap memory */
        files[count] = strdup(entry->d_name);
        count++;
    }

    if (errno != 0)
        perror("readdir failed");

    closedir(dp);

    if (count == 0)
        return;

    /* Sort filenames alphabetically */
    qsort(files, count, sizeof(char *), compare_strings);

    /* Display according to mode */
    if (long_format)
    {
        for (int i = 0; i < count; i++)
        {
            char fullpath[1024];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, files[i]);
            list_long(dir, fullpath);
        }
    }
    else if (horizontal_flag)
    {
        display_horizontal(files, count, dir);
    }
    else
    {
        display_columns(files, count, dir);
    }

    /* Free allocated memory */
    for (int i = 0; i < count; i++)
        free(files[i]);
}

/* =========================================================
 * display_columns()
 * Default ls output: "down then across" columns with colors
 * ========================================================= */
void display_columns(char **files, int count, const char *dir)
{
    if (count == 0)
        return;

    /* 1. Get terminal width */
    struct winsize w;
    int term_width = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0)
        term_width = w.ws_col;

    /* 2. Find the longest filename */
    int max_len = 0;
    for (int i = 0; i < count; i++)
    {
        int len = strlen(files[i]);
        if (len > max_len)
            max_len = len;
    }

    /* 3. Calculate columns and rows */
    int col_width = max_len + SPACING;
    int num_cols = term_width / col_width;
    if (num_cols < 1)
        num_cols = 1;

    int num_rows = (count + num_cols - 1) / num_cols;

    /* 4. Print row by row (down then across) */
    for (int row = 0; row < num_rows; row++)
    {
        for (int col = 0; col < num_cols; col++)
        {
            int index = col * num_rows + row;
            if (index >= count)
                continue;

            /* Get color for this file */
            char fullpath[1024];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, files[index]);

            struct stat st;
            const char *color = COLOR_RESET;
            if (lstat(fullpath, &st) == 0)
                color = get_color(files[index], &st);

            /* Print colored filename */
            printf("%s%-*s%s", color, col_width, files[index], COLOR_RESET);
        }
        printf("\n");
    }
}

/* =========================================================
 * display_horizontal()
 * -x output: files left-to-right, wrapping at width, with colors
 * ========================================================= */
void display_horizontal(char **files, int count, const char *dir)
{
    if (count == 0)
        return;

    /* 1. Get terminal width */
    struct winsize w;
    int term_width = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0)
        term_width = w.ws_col;

    /* 2. Find the longest filename */
    int max_len = 0;
    for (int i = 0; i < count; i++)
    {
        int len = strlen(files[i]);
        if (len > max_len)
            max_len = len;
    }

    /* 3. Column width */
    int col_width = max_len + SPACING;

    /* 4. Print and wrap at terminal width */
    int pos = 0;
    for (int i = 0; i < count; i++)
    {
        if (pos + col_width > term_width)
        {
            printf("\n");
            pos = 0;
        }

        /* Get color for this file */
        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, files[i]);

        struct stat st;
        const char *color = COLOR_RESET;
        if (lstat(fullpath, &st) == 0)
            color = get_color(files[i], &st);

        printf("%s%-*s%s", color, col_width, files[i], COLOR_RESET);
        pos += col_width;
    }
    printf("\n");
}

/* =========================================================
 * list_long()
 * Long listing format for one file, with colors
 * ========================================================= */
void list_long(const char *dir, const char *fullpath)
{
    (void)dir;

    struct stat st;
    if (lstat(fullpath, &st) == -1)
    {
        perror("lstat");
        return;
    }

    /* 1. Permissions string */
    char perm[11];
    format_permissions(st.st_mode, perm);

    /* 2. Owner name */
    struct passwd *pw = getpwuid(st.st_uid);
    const char *owner = pw ? pw->pw_name : "unknown";

    /* 3. Group name */
    struct group *gr = getgrgid(st.st_gid);
    const char *group = gr ? gr->gr_name : "unknown";

    /* 4. Modification time */
    char timebuf[64];
    strftime(timebuf, sizeof(timebuf), "%b %d %H:%M",
             localtime(&st.st_mtime));

    /* 5. Extract filename from full path */
    const char *filename = strrchr(fullpath, '/');
    filename = filename ? filename + 1 : fullpath;

    /* 6. Get color for the filename */
    const char *color = get_color(filename, &st);

    /* 7. Print all columns with colored filename */
    printf("%s %3ld %-8s %-8s %8ld %s %s%s%s\n",
           perm,
           (long)st.st_nlink,
           owner,
           group,
           (long)st.st_size,
           timebuf,
           color, filename, COLOR_RESET);
}

/* =========================================================
 * format_permissions()
 * Converts st_mode to "rwxr-xr-x" style string
 * ========================================================= */
void format_permissions(mode_t mode, char *str)
{
    /* File type */
    if (S_ISDIR(mode))       str[0] = 'd';
    else if (S_ISLNK(mode))  str[0] = 'l';
    else if (S_ISCHR(mode))  str[0] = 'c';
    else if (S_ISBLK(mode))  str[0] = 'b';
    else if (S_ISFIFO(mode)) str[0] = 'p';
    else if (S_ISSOCK(mode)) str[0] = 's';
    else                     str[0] = '-';

    /* User permissions */
    str[1] = (mode & S_IRUSR) ? 'r' : '-';
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    str[3] = (mode & S_IXUSR) ? 'x' : '-';

    /* Group permissions */
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    str[6] = (mode & S_IXGRP) ? 'x' : '-';

    /* Others permissions */
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    str[9] = (mode & S_IXOTH) ? 'x' : '-';

    str[10] = '\0';
}

/* =========================================================
 * get_color()
 * Returns the appropriate ANSI color code for a file
 * based on its type and name.
 *
 * Parameters:
 *   filename : name of the file
 *   st       : struct stat containing file metadata
 *
 * Returns:
 *   ANSI escape code string
 * ========================================================= */
const char *get_color(const char *filename, struct stat *st)
{
    /* 1. Symbolic link → Pink */
    if (S_ISLNK(st->st_mode))
        return COLOR_PINK;

    /* 2. Directory → Blue */
    if (S_ISDIR(st->st_mode))
        return COLOR_BLUE;

    /* 3. Special files (device, socket, fifo) → Reverse */
    if (S_ISCHR(st->st_mode) || S_ISBLK(st->st_mode) ||
        S_ISFIFO(st->st_mode) || S_ISSOCK(st->st_mode))
        return COLOR_REVERSE;

    /* 4. Executable (any execute bit) → Green */
    if (st->st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))
        return COLOR_GREEN;

    /* 5. Tarballs / archives by extension → Red */
    const char *ext = strrchr(filename, '.');
    if (ext != NULL)
    {
        if (strcmp(ext, ".tar") == 0 ||
            strcmp(ext, ".gz") == 0 ||
            strcmp(ext, ".zip") == 0 ||
            strcmp(ext, ".tgz") == 0 ||
            strcmp(ext, ".bz2") == 0 ||
            strcmp(ext, ".xz") == 0)
            return COLOR_RED;
    }

    /* 6. Default: no color */
    return COLOR_RESET;
}

/* =========================================================
 * compare_strings()
 * qsort() callback for alphabetical sort
 * ========================================================= */
int compare_strings(const void *a, const void *b)
{
    const char *str1 = *(const char **)a;
    const char *str2 = *(const char **)b;
    return strcmp(str1, str2);
}