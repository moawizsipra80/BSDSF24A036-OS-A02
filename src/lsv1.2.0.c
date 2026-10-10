/*
* Programming Assignment 02: lsv1.2.0
* Feature: Column Display (Down Then Across)
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

void do_ls(const char *dir, int long_format);
void list_long(const char *dir, const char *filename);
void format_permissions(mode_t mode, char *str);
int compare_strings(const void *a, const void *b);

void display_columns(char **files, int count)
{
    if (count == 0) return;

    struct winsize w;
    int term_width = 80;  /* default */
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0)
    {
        term_width = w.ws_col;
    }

    int max_len = 0;
    for (int i = 0; i < count; i++)
    {
        int len = strlen(files[i]);
        if (len > max_len) max_len = len;
    }

    int col_width = max_len + SPACING;
    int num_cols = term_width / col_width;
    if (num_cols < 1) num_cols = 1;

    int num_rows = (count + num_cols - 1) / num_cols;

    for (int row = 0; row < num_rows; row++)
    {
        for (int col = 0; col < num_cols; col++)
        {
            int index = col * num_rows + row;
            if (index >= count) continue;

            printf("%-*s", col_width, files[index]);
        }
        printf("\n");
    }
}

int main(int argc, char *argv[])
{
    int long_format = 0;
    int opt;
    int dir_count = 0;
    char *dirs[100];

    while ((opt = getopt(argc, argv, "l")) != -1)
    {
        if (opt == 'l')
            long_format = 1;
    }

    for (int i = optind; i < argc; i++)
    {
        dirs[dir_count++] = argv[i];
    }

    if (dir_count == 0)
    {
        do_ls(".", long_format);
    }
    else
    {
        for (int i = 0; i < dir_count; i++)
        {
            if (dir_count > 1)
                printf("%s:\n", dirs[i]);
            do_ls(dirs[i], long_format);
            if (i < dir_count - 1)
                puts("");
        }
    }
    return 0;
}

void do_ls(const char *dir, int long_format)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }

    char *files[MAX_FILES];
    int count = 0;

    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        if (count >= MAX_FILES) break;

        files[count] = strdup(entry->d_name);
        count++;
    }

    if (errno != 0)
        perror("readdir failed");

    closedir(dp);

    if (count == 0) return;

    qsort(files, count, sizeof(char *), compare_strings);

    if (long_format)
    {
        for (int i = 0; i < count; i++)
        {
            char fullpath[1024];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, files[i]);
            list_long(dir, fullpath);
        }
    }
    else
    {
        display_columns(files, count);
    }

    /* 4. Memory free karo */
    for (int i = 0; i < count; i++)
        free(files[i]);
}

void list_long(const char *dir, const char *fullpath)
{
    struct stat st;
    if (lstat(fullpath, &st) == -1)
    {
        perror("lstat");
        return;
    }

    char perm[11];
    format_permissions(st.st_mode, perm);

    struct passwd *pw = getpwuid(st.st_uid);
    char *owner = pw ? pw->pw_name : "unknown";

    struct group *gr = getgrgid(st.st_gid);
    char *group = gr ? gr->gr_name : "unknown";

    char timebuf[64];
    strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", localtime(&st.st_mtime));

    const char *filename = strrchr(fullpath, '/');
    filename = filename ? filename + 1 : fullpath;

    printf("%s %3ld %-8s %-8s %8ld %s %s\n",
           perm, (long)st.st_nlink, owner, group,
           (long)st.st_size, timebuf, filename);
}

void format_permissions(mode_t mode, char *str)
{
    if (S_ISDIR(mode))       str[0] = 'd';
    else if (S_ISLNK(mode))  str[0] = 'l';
    else if (S_ISCHR(mode))  str[0] = 'c';
    else if (S_ISBLK(mode))  str[0] = 'b';
    else if (S_ISFIFO(mode)) str[0] = 'p';
    else if (S_ISSOCK(mode)) str[0] = 's';
    else                     str[0] = '-';

    str[1] = (mode & S_IRUSR) ? 'r' : '-';
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    str[3] = (mode & S_IXUSR) ? 'x' : '-';
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    str[6] = (mode & S_IXGRP) ? 'x' : '-';
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    str[9] = (mode & S_IXOTH) ? 'x' : '-';
    str[10] = '\0';
}


int compare_strings(const void *a, const void *b)
{
    return strcmp(*(const char **)a, *(const char **)b);
}