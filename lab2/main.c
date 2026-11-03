#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>


typedef struct ThreadArgs {
    double *source;
    double *dest;
    double *filter;

    int size;
    int window;
    int iterations;

    int thread_id;
    int num_threads;

    pthread_barrier_t *barrier;
} ThreadArgs;


double get_pixel(double *matrix, int size, int i, int j) {
    if (i < 0 || i >= size || j < 0 || j >= size) {
        return 0.0;
    }
    return matrix[i * size + j];
}


double *use_filter(double *source, double *dest, double *filter, int size, int window, int iterations) {
    int half_w = window / 2;
    
    for (int iter = 0; iter < iterations; iter++) {

        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {     
                double sum = 0.0;

                for (int di = -half_w; di <= half_w; di++) {
                    for (int dj = -half_w; dj <= half_w; dj++) {
                        double pixel = get_pixel(source, size, i + di, j + dj);
                        int flt_idx = (di + half_w) * window + (dj + half_w);
                        
                        sum += pixel * filter[flt_idx];
                    }
                }
                
                dest[i * size + j] = sum;
            }
        }

        double *temp = source;
        source = dest;
        dest = temp;
    }
    return source;
}


void *thread_worker(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    
    int size = args->size;
    int window = args->window;
    int iterations = args->iterations;
    int thread_id = args->thread_id;
    int num_threads = args->num_threads;
    
    int half_w = window / 2;
    
    int start_row = (size * thread_id) / num_threads;
    int end_row = (size * (thread_id + 1)) / num_threads;
    
    double *curr_source = args->source;
    double *curr_dest = args->dest;
    double *curr_filter = args->filter;

    for (int iter = 0; iter < iterations; iter++) {
        for (int i = start_row; i < end_row; i++) {
            for (int j = 0; j < size; j++) {
                double sum = 0.0;
                
                for (int di = -half_w; di <= half_w; di++) {
                    for (int dj = -half_w; dj <= half_w; dj++) {
                        double pixel = get_pixel(curr_source, size, i + di, j + dj);
                        int flt_idx = (di + half_w) * window + (dj + half_w);
                        sum += pixel * curr_filter[flt_idx];
                    }
                }
                curr_dest[i * size + j] = sum;
            }
        }
        
        pthread_barrier_wait(args->barrier);

        double *temp = curr_source;
        curr_source = curr_dest;
        curr_dest = temp;
    }
    
    return curr_source;
}

double *parallel_use_filter(double *source, double *dest, double *filter, int size, int window, int iterations, int num_threads) {
    pthread_t *threads = malloc((size_t)num_threads * sizeof(pthread_t));
    ThreadArgs *args = malloc((size_t)num_threads * sizeof(ThreadArgs));
    
    if (threads == NULL || args == NULL) {
        printf("Ошибка выделения памяти для потоков\n");
        exit(1);
    }

    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, num_threads);

    for (int t = 0; t < num_threads; t++) {
        args[t].source = source;
        args[t].dest = dest;
        args[t].filter = filter;
        args[t].size = size;
        args[t].window = window;
        args[t].iterations = iterations;
        args[t].thread_id = t;
        args[t].num_threads = num_threads;
        args[t].barrier = &barrier;
        
        if (pthread_create(&threads[t], NULL, thread_worker, &args[t]) != 0) {
            printf("Ошибка создания потока %d\n", t);
            exit(1);
        }
    }

    void *result;
    pthread_join(threads[0], &result);

    for (int t = 1; t < num_threads; t++) {
        pthread_join(threads[t], NULL);
    }

    pthread_barrier_destroy(&barrier);
    free(threads);
    free(args);

    return (double *)result;   
}


int main(int argc, char *argv[]) {
    int size = 100;
    int window = 3;
    int iterations = 10;
    int threads = 1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--size") == 0 && i + 1 < argc) {
            size = atoi(argv[i + 1]);
            i++;
        }
        else if (strcmp(argv[i], "--window") == 0 && i + 1 < argc) {
            window = atoi(argv[i + 1]);
            i++;
        }
        else if (strcmp(argv[i], "--iterations") == 0 && i + 1 < argc) {
            iterations = atoi(argv[i + 1]);
            i++;
        }
        else if (strcmp(argv[i], "--threads") == 0 && i + 1 < argc) {
            threads = atoi(argv[i + 1]);
            i++;
        }
        else {
            printf("Неизвестный аргумент: %s\n", argv[i]);
            return 1;
        }
    }

    if (size <= 0 || size > 10000) {
        printf("Размер матрицы должен быть от 1 до 10000\n");
        return 1;
    }

    if (window <= 0 || window % 2 == 0 || window > 11) {
        printf("Размер фильтра должен быть нечётным числом от 1 до 11\n");
        return 1;
    }
    if (window >= size) {
        printf("Размер фильтра должен быть строго меньше размера матрицы\n");
        return 1;
    }

    if (iterations <= 0 || iterations > 10000) {
        printf("Количество итераций должно быть от 1 до 10000\n");
        return 1;
    }

    if (threads <= 0 || threads > 10000) {
        printf("Количество потоков должно быть от 1 до 10000\n");
        return 1;
    }


    double *source = (double *)malloc((size_t)size * size * sizeof(double));
    double *dest = (double *)malloc((size_t)size * size * sizeof(double));

    if (source == NULL || dest == NULL) {
        printf("Не удалось выделить память под матрицы\n");
        if (source) free(source);
        if (dest) free(dest);
        return 1;
    }

    double *filter = (double *)malloc((size_t)window * window * sizeof(double));
    if (filter == NULL) {
        printf("Не удалось выделить память под фильтр\n");
        free(source);
        free(dest);
        return 1;
    }

    srand(42);
    for (int i = 0; i < size * size; i++) {
        source[i] = (double)rand() / RAND_MAX;
    }

    double filter_value = 1.0 / (window * window);
    for (int i = 0; i < window * window; i++) {
        filter[i] = filter_value;
    }

    if (size <= 5) {
        printf("Исходная матрица (%dx%d)\n", size, size);
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                printf("%.4f\t", source[i * size + j]);
            }
            printf("\n");
        }

        printf("Фильтр (%dx%d)\n", window, window);
        for (int i = 0; i < window; i++) {
            for (int j = 0; j < window; j++) {
                printf("%.4f\t", filter[i * window + j]);
            }
            printf("\n");
        }
    }

    struct timespec start_seq, end_seq;
    clock_gettime(CLOCK_MONOTONIC, &start_seq);

    double *result = use_filter(source, dest, filter, size, window, iterations);

    clock_gettime(CLOCK_MONOTONIC, &end_seq);

    double time_seq = (end_seq.tv_sec - start_seq.tv_sec) * 1000.0 + (end_seq.tv_nsec - start_seq.tv_nsec) / 1000000.0;

    printf("Последовательная версия: %.3f миллисекунд\n", time_seq);

    if (size <= 5) {
        printf("Итоговая матрица после последовательной свертки (%dx%d)\n", size, size);
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                printf("%.4f\t", result[i * size + j]);
            }
            printf("\n");
        }
    }


    printf("\n");
    srand(42);
    for (int i = 0; i < size * size; i++) {
        source[i] = (double)rand() / RAND_MAX;
    }

    struct timespec start_par, end_par;
    clock_gettime(CLOCK_MONOTONIC, &start_par);

    double *par_result = parallel_use_filter(source, dest, filter, size, window, iterations, threads);

    clock_gettime(CLOCK_MONOTONIC, &end_par);

    printf("Параллельная версия (%d потоков):\n", threads);
    if (size <= 5) {
        printf("Итоговая матрица после параллельной свертки (%dx%d)\n", size, size);
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                printf("%.4f\t", par_result[i * size + j]);
            }
            printf("\n");
        }
    }

    double time_par = (end_par.tv_sec - start_par.tv_sec) * 1000.0 + (end_par.tv_nsec - start_par.tv_nsec) / 1000000.0;
    double speedup = time_seq / time_par;
    double efficiency = speedup / threads;

    printf("Время: %.3f мс\n", time_par);
    printf("Ускорение: %.3f\n", speedup);
    printf("Эффективность: %.3f\n", efficiency);

    free(source);
    free(dest);
    free(filter);
    return 0;
}
