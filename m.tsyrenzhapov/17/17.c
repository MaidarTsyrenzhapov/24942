#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define MAX_LINE 40 //backspace
#define ERASE 0x7F //ctrl + u
#define KILL 0x15
#define CTRL_W 0x17
#define CTRL_D 0x04
#define CTRL_G 0x07

struct termios orig_attrs;

void restore_terminal(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_attrs);
}

void setup_terminal(void) {
    struct termios attrs;
    tcgetattr(STDIN_FILENO, &orig_attrs);   
    atexit(restore_terminal);
    
    attrs = orig_attrs;
    attrs.c_lflag &= ~(ICANON | ECHO);
    attrs.c_cc[VMIN] = 1;
    attrs.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &attrs);
}

int is_printable(char c) {
    return c >= 32 && c <= 126;
}

int find_word_start(char *line, int pos) {
    if (pos == 0) return 0;
    
    int i = pos - 1;
    // Пропускаем пробелы
    while (i >= 0 && line[i] == ' ') i--;
    // Идем до начала слова
    while (i >= 0 && line[i] != ' ') i--;
    
    return i + 1;
}

int main(void) {
    char line[1024] = {0};
    int pos = 0;
    int current_line_length = 0;
    char c;
    char beep = CTRL_G;  // Создаем переменную для звукового сигнала
    
    setup_terminal();
    printf("Line editor (Ctrl+D to exit): ");
    fflush(stdout);
    current_line_length = 24; // Длина приглашения

    while (1) {
        read(STDIN_FILENO, &c, 1);
        
        // Выход по Ctrl+D только в начале строки
        if (c == CTRL_D && pos == 0) {
            printf("\n");
            break;
        }
        
        // Проверяем допустимые символы
        if (!is_printable(c) && c != ERASE && c != KILL && c != CTRL_W) {
            write(STDOUT_FILENO, &beep, 1); // Звуковой сигнал
            continue;
        }
        
        switch (c) {
            case ERASE: // Backspace
                if (pos > 0) {
                    pos--;
                    current_line_length--;
                    // Удаляем символ с экрана
                    printf("\b \b");
                    fflush(stdout);
                } else {
                    write(STDOUT_FILENO, &beep, 1);
                }
                break;
                
            case KILL: // Очистка всей строки
                if (pos > 0) {
                    // Возвращаемся в начало строки
                    for (int i = 0; i < current_line_length; i++) {
                        printf("\b \b");
                    }
                    pos = 0;
                    current_line_length = 0;
                    fflush(stdout);
                } else {
                    write(STDOUT_FILENO, &beep, 1);
                }
                break;
                
            case CTRL_W: // Удаление последнего слова
                if (pos > 0) {
                    int word_start = find_word_start(line, pos);
                    int chars_to_delete = pos - word_start;
                    
                    // Удаляем с экрана
                    for (int i = 0; i < chars_to_delete; i++) {
                        printf("\b \b");
                        current_line_length--;
                    }
                    pos = word_start;
                    fflush(stdout);
                } else {
                    write(STDOUT_FILENO, &beep, 1);
                }
                break;
                
            default: // Обычный символ
                // Проверяем ограничение длины
                if (current_line_length >= MAX_LINE) {
                    // Ищем последний пробел для переноса
                    int last_space = -1;
                    for (int i = pos - 1; i >= 0; i--) {
                        if (line[i] == ' ') {
                            last_space = i;
                            break;
                        }
                    }
                    
                    if (last_space != -1) {
                        // Переносим на новую строку
                        printf("\n");
                        // Выводим оставшуюся часть слова
                        for (int i = last_space + 1; i < pos; i++) {
                            write(STDOUT_FILENO, &line[i], 1);
                        }
                        // Выводим новый символ
                        write(STDOUT_FILENO, &c, 1);
                        line[pos++] = c;
                        current_line_length = (pos - last_space - 1) + 1;
                    } else {
                        // Нет пробелов - переносим символ на новую строку
                        printf("\n");
                        write(STDOUT_FILENO, &c, 1);
                        line[pos++] = c;
                        current_line_length = 1;
                    }
                } else {
                    // Обычный вывод
                    write(STDOUT_FILENO, &c, 1);
                    line[pos++] = c;
                    current_line_length++;
                }
                break;
        }
        
        line[pos] = '\0';
        fflush(stdout);
    }
    
    return 0;
}