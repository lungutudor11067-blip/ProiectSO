#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

typedef struct {
    int id;
    char inspector[64];
    double x,y;
    char category[32];
    int severity;
    time_t timestamp;
    char description[256];
}Report;

char user[64];
char role[16];

void create_district(const char *district){
    if(mkdir(district,0750)==-1 && errno!=EEXIST){
        perror(NULL);
        return;
    }
    char path[256];
    int f;
    snprintf(path, sizeof(path), "%s/reports.dat", district);
    f=open(path, O_WRONLY | O_CREAT | O_EXCL, 0664);
    if(f!=-1){
        close(f);
        chmod(path, 0664);
    }
    else if(errno!=EEXIST){
        perror(NULL);
    }
    snprintf(path, sizeof(path), "%s/district.cfg", district);
    f=open(path, O_WRONLY | O_CREAT | O_EXCL, 0640);
    if(f!=-1){
        write(f, "threshold=1\n", 12);
        close(f);
        chmod(path, 0640);
    }
    else if(errno!=EEXIST){
        perror(NULL);
    }
    snprintf(path, sizeof(path), "%s/logged_district", district);
    f=open(path, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if(f!=-1){
        close(f);
        chmod(path, 0644);
    }
    else if(errno!=EEXIST){
        perror(NULL);
    }
}

void add_aux(const char *district, const char *action){
    char path[256];
    snprintf(path, sizeof(path), "%s/logged_district", district);
    int f=open(path, O_WRONLY | O_APPEND);
    if (f==-1) {
        perror(NULL);
        return;
    }
    char entry[256];
    int len=snprintf(entry, sizeof(entry), "%ld %s %s %s\n",(long)time(NULL), user, role, action);
    write(f, entry, len);
    close(f);
}

int get_id(const char *district){
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);
    int f=open(path, O_RDONLY);
    if(f==-1)
        return 100;
    int max_id=99;
    Report r;
    while(read(f, &r, sizeof(Report))>0){
        if(r.id>max_id)
            max_id=r.id;
    }
    close(f);
    return max_id+1;
}

void add(const char *district) {
    struct stat st;
    if(stat(district, &st)==-1) {
         create_district(district);
    }
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);
    if(stat(path, &st)==0){
        if((st.st_mode & 0777)!=0664){
            fprintf(stderr, "Eroare permisiuni pe %s!\n", path);
            return;
        }
    }
    Report r;
    memset(&r, 0, sizeof(Report));
    r.id=get_id(district);
    strncpy(r.inspector, user, sizeof(r.inspector)-1);
    printf("Latitude: ");
    scanf("%lf", &r.x);
    printf("Longitude: ");
    scanf("%lf", &r.y);
    printf("Category (road/lighting/flooding): ");
    scanf("%s", r.category);
    printf("Severity (1=minor, 2=moderate, 3=critical): ");
    scanf("%d", &r.severity);
    r.timestamp=time(NULL);
    printf("Description: ");
    getchar();
    fgets(r.description, sizeof(r.description), stdin);
    r.description[strcspn(r.description, "\n")]=0;
    int f = open(path, O_WRONLY | O_APPEND);
    if (f==-1){
        perror(NULL);
        return;
    }
    if(write(f, &r, sizeof(Report))!=-1){
        add_aux(district, "add");
    }
    close(f);
}

void print_perms(mode_t mode){
    char p[10] = "---------";
    if(mode & S_IRUSR)
        p[0]='r';
    if(mode & S_IWUSR)
        p[1]='w';
    if(mode & S_IXUSR)
        p[2]='x';
    if(mode & S_IRGRP)
        p[3]='r';
    if(mode & S_IWGRP)
        p[4]='w';
    if(mode & S_IXGRP)
        p[5]='x';
    if(mode & S_IROTH)
        p[6]='r';
    if(mode & S_IWOTH)
        p[7]='w';
    if(mode & S_IXOTH)
        p[8]='x';
    printf("%s", p);
}
void list(char *district) {
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);
    struct stat st;
    if(stat(path, &st)==-1){
        perror(NULL);
        return;
    }
    printf("File: %s   Size: %lld   Perms: ", path, (long long)st.st_size);
    print_perms(st.st_mode);
    printf("\nLast modification: %s", ctime(&st.st_mtime));
    int f=open(path, O_RDONLY);
    if(f==-1){
        perror(NULL);
        return;
    }
    Report r;
    while(read(f, &r, sizeof(Report))>0) {
        printf("ID: %d | Cat: %s | Severity: %d\n", r.id, r.category, r.severity);
    }
    close(f);
    add_aux(district, "list");
}

void view(const char *district, int id) {
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);
    int f=open(path, O_RDONLY);
    if(f==-1) {
        perror(NULL);
        return;
    }
    Report r;
    int ok=0;
    while(read(f, &r, sizeof(Report))>0) {
        if(r.id==id){
            printf("Raport ID: %d\n", r.id);
            printf("Inspector: %s\n", r.inspector);
            printf("Latitudine %.4f, Longitude %.4f\n", r.x, r.y);
            printf("Categorie: %s\n", r.category);
            printf("Severitate: %d\n", r.severity);
            printf("Timestamp: %ld\n", r.timestamp);
            printf("Descriere: %s\n", r.description);
            ok=1;
            add_aux(district, "view");
            break;
        }
    }
    if (!ok) {
        printf("Raportul nu a fost gasit\n");
    }
    close(f);
}

void remove_report(const char *district, int id){
    if(strcmp(role, "manager")!=0){
        printf("Actiune nepermisa\n");
        return;
    }
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);
    int f=open(path, O_RDWR);
    if(f==-1) {
        perror(NULL);
        return;
    }
    Report r;
    long offset=-1;
    int ok=0;
    while(read(f, &r, sizeof(Report))>0) {
        if(r.id==id){
            offset=lseek(f, 0, SEEK_CUR)-sizeof(Report);
            ok=1;
            break;
        }
    }
    if(!ok) {
        close(f);
        return;
    }
    long cur_pos=offset+sizeof(Report);
    while(lseek(f, cur_pos, SEEK_SET)!=-1 && read(f, &r, sizeof(Report))>0){
        lseek(f, -2*(long)sizeof(Report), SEEK_CUR);
        write(f, &r, sizeof(Report));
        cur_pos+=sizeof(Report);
        lseek(f, cur_pos, SEEK_SET);
    }
    ftruncate(f, lseek(f, 0, SEEK_CUR)-sizeof(Report));
    add_aux(district, "remove_report");
    close(f);
}

int main(int argc, char **argv){
    if(argc<7){
        fprintf(stderr,"Prea putine argumente");
        exit(-1);
    }
    strncpy(role,argv[2],sizeof(role)-1);
    strncpy(user,argv[4],sizeof(user)-1);
    if(strcmp(argv[5], "--add")==0){
        add(argv[6]);
    }
    else if(strcmp(argv[5], "--list")==0){
        list(argv[6]);
    }
    else if(strcmp(argv[5], "--view")==0){
        if(argc<8){
            fprintf(stderr,"Prea putine argumente");
            exit(-1);
        }
        char *endptr;
        long val=strtol(argv[7],&endptr,10);
        if(*endptr!='\0'){
            fprintf(stderr,"Argument incorect introdus");
            exit(-1);
        }
        view(argv[6], (int)val);
    }
    else if(strcmp(argv[5], "--remove_report")==0){
        if(argc<8){
            fprintf(stderr,"Prea putine argumente");
            exit(-1);
        }
        char *endptr;
        long val=strtol(argv[7],&endptr,10);
        if(*endptr!='\0'){
            fprintf(stderr,"Argument incorect introdus");
            exit(-1);
        }
        remove_report(argv[6], (int)val);
    }
    return 0;
}
