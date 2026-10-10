/*
 * Programming Assignment 02: lsv1.3.0
 * Feature: Horizontal Column Display (-x option)
 *
 * This version adds:
 *   -l : Long listing format
 *   -x : Horizontal (across) column display
 *   default : Down-then-across column display
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

/* Function prototypes */
void do_ls(const char *dir, int long_format, int horizontal_flag);
void list_long(const char *dir, const char *fullpath);
void format_permissions(mode_t mode, char *str);
int compare_strings(const void *a, const void *b);
void display_columns(char **files, int count);
void display_horizontal(char **files, int count);

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

    /*
     * Sort the filenames array alphabetically.
     *
     * qsort signature: qsort(base, num, size, compare_fn)
     *   base      : Pointer to array
     *   num       : Number of elements
     *   size      : Size of each element (sizeof(char*) = 8 bytes)
     *   compare_fn: Pointer to comparison function
     * */
    qsort(files, count, sizeof(char *), compare_strings);
    /* Display according to mode */
    if (long_format)
    {
        /* -l : Long listing format */
        for (int i = 0; i < count; i++)
        {
            char fullpath[1024];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, files[i]);
            list_long(dir, fullpath);
        }
    }
    else if (horizontal_flag)
    {
        /* -x : Horizontal (across) display */
        display_horizontal(files, count);
    }
    else
    {
        /* default : Down-then-across columns */
        display_columns(files, count);
    }

    /* =========================================================
     * Free allocated memory.
     *
     * Why?
     *   - Each filename was strdup()'d (heap allocation)
     *   - We must free them to avoid memory leaks
     *   - The array itself is on stack, so no need to free it
     * ========================================================= */
    for (int i = 0; i < count; i++)
        free(files[i]);
}

/* =========================================================
 * display_columns()
 * Default ls output: "down then across" columns
 * ========================================================= */
void display_columns(char **files, int count)
{
    if (count == 0)
        return;

    /* 1. Get terminal width */
    struct winsize w;
    int term_width = 80; /* fallback */
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

    /* 3. Calculate number of columns */
    int col_width = max_len + SPACING;
    int num_cols = term_width / col_width;
    if (num_cols < 1)
        num_cols = 1;

    /* 4. Calculate number of rows */
    int num_rows = (count + num_cols - 1) / num_cols;

    /* 5. Print row by row (down then across) */
    for (int row = 0; row < num_rows; row++)
    {
        for (int col = 0; col < num_cols; col++)
        {
            int index = col * num_rows + row;
            if (index >= count)
                continue;
            printf("%-*s", col_width, files[index]);
        }
        printf("\n");
    }
}

/* =========================================================
 * display_horizontal()
 * -x output: files listed left-to-right, wrapping at width
 * ========================================================= */
void display_horizontal(char **files, int count)
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
        printf("%-*s", col_width, files[i]);
        pos += col_width;
    }
    printf("\n");
}

/* =========================================================
 * list_long()
 * Long listing format for one file
 * ========================================================= */
void list_long(const char *dir, const char *fullpath)
{
    (void)dir; /* unused, kept for signature compatibility */

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

    /* 6. Print all columns */
    printf("%s %3ld %-8s %-8s %8ld %s %s\n",
           perm,
           (long)st.st_nlink,
           owner,
           group,
           (long)st.st_size,
           timebuf,
           filename);
}

/* =========================================================
 * format_permissions()
 * Converts st_mode to "rwxr-xr-x" style string
 * ========================================================= */
void format_permissions(mode_t mode, char *str)
{
    /* File type */
    if (S_ISDIR(mode))
        str[0] = 'd';
    else if (S_ISLNK(mode))
        str[0] = 'l';
    else if (S_ISCHR(mode))
        str[0] = 'c';
    else if (S_ISBLK(mode))
        str[0] = 'b';
    else if (S_ISFIFO(mode))
        str[0] = 'p';
    else if (S_ISSOCK(mode))
        str[0] = 's';
    else
        str[0] = '-';

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
 * compare_strings()
 * qsort() callback for alphabetical sort
 * ========================================================= */
/* =========================================================
 * compare_strings()
 *
 * qsort() callback function for alphabetical sort.
 *
 * Parameters:
 *   a, b : Pointers to elements being compared.
 *          Since our array is char*[], we cast them to char**.
 *
 * Return:
 *   < 0 : a comes before b
 *   = 0 : a and b are equal
 *   > 0 : a comes after b
 *
 * Why const void*?
 *   qsort() is generic — it works with any data type.
 *   So it uses void* to accept any pointer type.
 *   We cast to char** because our array holds char* elements.
 * ========================================================= */
int compare_strings(const void *a, const void *b)
{
    const char *str1 = *(const char **)a;
    const char *str2 = *(const char **)b;
    return strcmp(str1, str2);
}