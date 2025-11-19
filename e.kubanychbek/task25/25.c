#include <stdio.h> 
#include <unistd.h>
#include <ctype.h> 
#include <string.h> 

int main(void){
    int fd[2];
    pipe(fd); //создаем pipe: fd[0] - чтение, fd[1] - запись
    
    if (fork() == 0){
        close(fd[1]); //закрываем конец для записи 
        
        char buf[256];
        int n;

        while ((n = read(fd[0], buf, sizeof(buf))) > 0){
            //переводим каждый символ в верхний регистр
            for (int i = 0; i < n; i++){
                buf[i] = toupper((unsigned char) buf[i]);
            }
            write(1, buf, n); //вывод на экран 
        }
        close(fd[0]);
        return 0; 
    }

    //родительский процесс
    close(fd[0]); // закрываем конец для чтения 
    const char *text = "Сжатие объектов: 100 (4/4), готово.\nЗапись объектов: 100 (5/5), 921 байт | 921.00 КиБ/с, готово.\nTotal 5 (delta 2), reused 0 (delta 0), pack-reused 0 (from 0)remote: \nResolving deltas: 100 (2/2), completed with 2 local objects.\nTo github.com:Erbol2006/24942.git1822a10..221bec5  main -> main\nThis is Mixed Case TeXT.\n";
    write(fd[1], text, strlen(text));
    close(fd[1]); //закрываем pipe, то есть дочерний процеес увидит EOF
    return 0;
}