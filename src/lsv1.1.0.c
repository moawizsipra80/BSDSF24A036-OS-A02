/*
* Programming Assignment 02: lsv1.1.0
* This is the source file of version 1.1.0
* Feature: Long Listing Format (-l)
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

extern int errno;

void do_ls(const char *dir, int long_format);
void list_long(const char *dir, const char *filename);
void format_permissions(mode_t mode, char *str);

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

    /* Arguments collect karo (flags ke baad) */
    for (int i = optind; i < argc; i++)
    {
        dirs[dir_count++] = (char *)argv[i];
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
    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        if (long_format)
        {
            char fullpath[1024];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, entry->d_name);
            list_long(dir, fullpath);
        }
        else
        {
            printf("%s\n", entry->d_name);
        }
    }

    if (errno != 0)
        perror("readdir failed");

    closedir(dp);
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

    nlink_t links = st.st_nlink;

    struct passwd *pw = getpwuid(st.st_uid);
    char *owner = pw ? pw->pw_name : "unknown";

    struct group *gr = getgrgid(st.st_gid);
    char *group = gr ? gr->gr_name : "unknown";

    off_t size = st.st_size;

    char timebuf[64];
    strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", localtime(&st.st_mtime));

    const char *filename = strrchr(fullpath, '/');
    filename = filename ? filename + 1 : fullpath;

    printf("%s %3ld %-8s %-8s %8ld %s %s\n",
           perm, (long)links, owner, group, (long)size, timebuf, filename);
}

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