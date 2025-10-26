#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wiringPi.h>
#include <time.h>
#include <sqlite3.h>
#include "l_dht22.h"
#include "oled_display.h"
#include <unistd.h>

#define DB_FILE "dht22_data.db"

// Estrutura para estatísticas de CPU
typedef struct {
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
} CPUStats;

void read_cpu_stats(CPUStats *stats) {
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) return;
    fscanf(fp, "cpu  %llu %llu %llu %llu %llu %llu %llu %llu",
           &stats->user, &stats->nice, &stats->system, &stats->idle,
           &stats->iowait, &stats->irq, &stats->softirq, &stats->steal);
    fclose(fp);
}

float get_cpu_usage() {
    static CPUStats prev = {0};
    CPUStats curr;
    read_cpu_stats(&curr);

    unsigned long long prev_idle = prev.idle + prev.iowait;
    unsigned long long curr_idle = curr.idle + curr.iowait;

    unsigned long long prev_total = prev.user + prev.nice + prev.system + prev_idle +
                                    prev.irq + prev.softirq + prev.steal;
    unsigned long long curr_total = curr.user + curr.nice + curr.system + curr_idle +
                                    curr.irq + curr.softirq + curr.steal;

    unsigned long long total_diff = curr_total - prev_total;
    unsigned long long idle_diff = curr_idle - prev_idle;

    prev = curr;

    if (total_diff == 0) return 0.0;
    return (float)(total_diff - idle_diff) * 100.0 / total_diff;
}

float get_cpu_temp() {
    FILE *fp = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (!fp) return -1;
    int temp_miligrados;
    fscanf(fp, "%d", &temp_miligrados);
    fclose(fp);
    return temp_miligrados / 1000.0;
}

int main() {
    if (wiringPiSetup() == -1) { 
        printf("Falha ao inicializar WiringPi\n"); 
        return -1; 
    }

    int fd = wiringPiI2CSetup(enderecoI2C);
    if (fd == -1) { 
        printf("Falha ao inicializar I2C\n"); 
        return -1; 
    }
    inicializaDisplay(fd);
    limpaDisplay(fd);

    sqlite3 *db;
    char *err_msg = 0;
    if (sqlite3_open(DB_FILE, &db)) { 
        printf("Erro ao abrir banco: %s\n", sqlite3_errmsg(db)); 
        return -1; 
    }

    const char *sql_create =
        "CREATE TABLE IF NOT EXISTS leitura ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "datahora TEXT, "
        "temperatura REAL, "
        "umidade REAL, "
        "temp_cpu REAL);";
    if (sqlite3_exec(db, sql_create, 0, 0, &err_msg) != SQLITE_OK) {
        printf("Erro SQL: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return -1;
    }

    // Fuso horário brasileiro
    setenv("TZ", "America/Sao_Paulo", 1);
    tzset();

    int temp, hum;
    double tempC = 0, humPerc = 0;
    float cpu_temp = 0, cpu_usage = 0;
    char texto[32];

    time_t last_db_time = 0;
    int last_second = -1; // para sincronizar OLED com segundos

    while (1) {
        if (read_dht22(&temp, &hum) == 0) {
            tempC = temp / 10.0;
            humPerc = hum / 10.0;
            cpu_temp = get_cpu_temp();
            cpu_usage = get_cpu_usage();

            time_t t = time(NULL);
            struct tm tm_info;
            localtime_r(&t, &tm_info);
            char datetime[32];
            strftime(datetime, sizeof(datetime), "%d/%m/%Y %H:%M:%S", &tm_info);

            // Atualiza SQLite a cada 10 segundos
            if (difftime(t, last_db_time) >= 10.0) {
                char sql_insert[256];
                snprintf(sql_insert, sizeof(sql_insert),
                         "INSERT INTO leitura (datahora, temperatura, umidade, temp_cpu) "
                         "VALUES ('%s', %.1f, %.1f, %.1f);",
                         datetime, tempC, humPerc, cpu_temp);
                if (sqlite3_exec(db, sql_insert, 0, 0, &err_msg) != SQLITE_OK) {
                    printf("Erro SQL insert: %s\n", err_msg);
                    sqlite3_free(err_msg);
                }
                last_db_time = t;
            }

            // Só atualiza OLED se o segundo mudou
            if (tm_info.tm_sec != last_second) {
                last_second = tm_info.tm_sec;

                // Linha 0: data e hora
                snprintf(texto, sizeof(texto), "%s", datetime);
                escreveTexto(fd, texto, 0);

                // Linha 1: temperatura
                snprintf(texto, sizeof(texto), "Temp: %.1f C", tempC);
                escreveTexto(fd, texto, 1);

                // Linha 2: umidade
                snprintf(texto, sizeof(texto), "Umid: %.1f %%", humPerc);
                escreveTexto(fd, texto, 2);

                // Linha 3: CPU temp e uso
                snprintf(texto, sizeof(texto), "CPU: %.1fC Uso: %.1f%%", cpu_temp, cpu_usage);
                escreveTexto(fd, texto, 3);

                // Terminal
                printf("\r%s  Temp: %.1fC  Umid: %.1f%%  CPU: %.1fC  Uso CPU: %.1f%%",
                       datetime, tempC, humPerc, cpu_temp, cpu_usage);
                fflush(stdout);
            }
        }

        delay(200); // Loop rápido para checar segundos
    }

    sqlite3_close(db);
    return 0;
}
