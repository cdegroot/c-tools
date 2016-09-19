#include <io.h>
#include <fcntl.h>
#include <dos.h>
#include <dir.h>
#include <stdio.h>
#include <toolset.h>
#include <btree.h>
/*
	$Id: unlock.c,v 1.2 1993/02/05 13:03:04 cg Rel $
*/

void unl_ndx(char *name);

int main(int argc, char **argv)
{
    register char *s;
    register int result;
    struct ffblk fcb;
    char oldpath[MAXPATH], newpath[MAXPATH];
    char drive[MAXDRIVE];
    char dir[MAXDIR];
    char file[MAXFILE];
    char ext[MAXEXT];
    char name[MAXFILE+MAXEXT];

    /* Check aantal argumenten */
    if (argc != 2)
    {
        printf("Index name required.\n");
        return 1;
    }

    /* Check for wildcards */
    s = argv[1];
    while (*s)
    {
        if (*s == '*' || *s == '?')
            break;
        s++;
    }
    if (!*s)
    {
        /* No wildcards found */
        unl_ndx(argv[1]);
        return 0;
    }
    else
    {
        /* See if a path is specified */
        result = fnsplit(argv[1], drive, dir, file, ext);
        if (result & DIRECTORY || result & DRIVE)
        {
            /* There was a pathname, save cwd and make new dir */
            getcwd(oldpath,MAXPATH);
            fnmerge(newpath,drive,dir,"\0","\0");

            /* Delete trailing backslash, if necessary */
            if (strlen(newpath) > 1)
                *(newpath+strlen(newpath)-1) = '\0';

            /* Try to change directory */
            if (chdir(newpath) == -1)
            {
                printf("Specified path %s not found\n",newpath);
                return 1;
            }
        }

        /* Merge file & ext */
        strcpy(name,file);
        strcat(name,ext);

        /* Process wildcards */
        if (findfirst(name,&fcb,0))
        {
            /* No matching names found */
            printf("No files found matching %s\n",argv[1]);
        }
        else
        {
            do
                /* Unlock file */
                unl_ndx(fcb.ff_name);
            while (!findnext(&fcb)); /* While findnext returns zero */
        }

        /* Check too see if path has to be changed back */
        if (result & DIRECTORY || result & DRIVE)
            chdir(oldpath);
    }
    return 0;
}

void unl_ndx(char *name)
{
    int fd;
    struct b_hdr r;

    /* Open specified file */
    if ((fd = _open(name, O_RDWR)) == ERROR)
    {
        printf("\nNo index named %s can be found\n",name);
        return;
    }

    /* Print message and read header */
    printf("Unlocking indexfile %12s: ",name);
    if (_read(fd, &r, sizeof(r)) == -1)
    {
        printf("error reading file\n");
        return;
    }

    /* Check if index is locked */
    if (!r.in_use)
        printf("file was not locked\n");
    else
    {
        /* Reset byte */
        r.in_use = 0;

        /* Write header */
        if (lseek(fd, 0, 0) == -1)
            printf("error in seek\n");
        else if (_write(fd, &r, sizeof(r)) == -1)
            printf("error writing file\n");
        else
            printf("now unlocked\n");
    }

    /* Close file */
    close(fd);
    return;
}
