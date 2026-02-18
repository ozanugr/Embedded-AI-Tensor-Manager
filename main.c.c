/*
 * Project: Embedded AI Tensor Manager
 * Author: [Samet Ozan Ugur]
 * Note: Agentic Coding yontemiyle (Gemini & GPT) gelistirilmistir.
 * IDE: Dev-C++ / GCC
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <float.h>
#include <time.h>

// --- MACROS ---
// Fonksiyon overhead'inden kurtulmak icin macro tanimladim (Islemci dostu)
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#define MAX_Q 127
#define MIN_Q -128

// --- DATA STRUCTURES ---

typedef enum {
    MODE_FLOAT32, // Standart hassasiyet
    MODE_INT8     // Quantized (Sikistirilmis)
} BufferMode;

// RAM tasarrufu icin Union yapisi 
// (Ayni bellek adresini paylasiyorlar)
typedef union {
    float* ptr_f;   // 32-bit veri
    int8_t* ptr_i;  // 8-bit veri
} MemoryUnion;

// Ana Tensor Yapisi (MicroBuffer)
typedef struct {
    int* dims;          // Boyutlar (Dimensions)
    int dim_count;      // Rank sayisi
    int total_size;     // Toplam eleman
    
    BufferMode mode;    // Aktif mod
    MemoryUnion mem;    // Veri blogu
    
    // Quantization parametreleri
    float scale_factor;
    int32_t zero_pt;
    
} MicroBuffer;

// --- FUNCTIONS ---

// Dinamik buffer olustur (Dynamic Allocation)
MicroBuffer* init_buffer(int* dimensions, int rank, BufferMode mode) {
    int i; 
    MicroBuffer* mb = (MicroBuffer*)malloc(sizeof(MicroBuffer));
    
    if (mb == NULL) {
        printf("[ERR] Yetersiz Bellek!\n");
        return NULL;
    }

    mb->dim_count = rank;
    mb->mode = mode;
    // calloc kullaniyorum ki garbage value olusmasin
    mb->dims = (int*)calloc(rank, sizeof(int)); 
    mb->total_size = 1;

    for (i = 0; i < rank; i++) {
        mb->dims[i] = dimensions[i];
        mb->total_size *= dimensions[i];
    }

    // Mod'a gore allocation yap
    if (mode == MODE_FLOAT32) {
        mb->mem.ptr_f = (float*)calloc(mb->total_size, sizeof(float));
        mb->scale_factor = 0.0f;
        mb->zero_pt = 0;
        if (!mb->mem.ptr_f) return NULL;
    } else {
        mb->mem.ptr_i = (int8_t*)calloc(mb->total_size, sizeof(int8_t));
        if (!mb->mem.ptr_i) return NULL;
    }

    return mb;
}

// Memory leak onlemek icin temizlik
void destroy_buffer(MicroBuffer* mb) {
    if (mb != NULL) {
        if (mb->dims) free(mb->dims);
        
        // Aktif pointer'i serbest birak
        if (mb->mode == MODE_FLOAT32 && mb->mem.ptr_f) 
            free(mb->mem.ptr_f);
        else if (mb->mode == MODE_INT8 && mb->mem.ptr_i) 
            free(mb->mem.ptr_i);
            
        free(mb);
        printf(">> Bellek temizlendi (Free).\n");
    }
}

// Float -> Int8 Donusumu (Quantization Logic)
MicroBuffer* compress_to_int8(MicroBuffer* source) {
    int i;
    float min_v = FLT_MAX;
    float max_v = -FLT_MAX;
    float val;
    int32_t raw_q;
    
    MicroBuffer* q_buf;

    if (source->mode != MODE_FLOAT32) return NULL;

    // 1. Min/Max bul
    for (i = 0; i < source->total_size; i++) {
        val = source->mem.ptr_f[i];
        if (val < min_v) min_v = val;
        if (val > max_v) max_v = val;
    }
    
    // Zero division hatasini onle
    if (max_v == min_v) max_v += 0.001f;

    // 2. Hedef Buffer'i hazirla
    q_buf = init_buffer(source->dims, source->dim_count, MODE_INT8);
    if (!q_buf) return NULL;

    // 3. Scale ve Zero Point hesabi (Asymmetric Quantization)
    q_buf->scale_factor = (max_v - min_v) / 255.0f;
    q_buf->zero_pt = (int32_t)round(-min_v / q_buf->scale_factor) - 128;

    printf("\n[QUANTIZATION INFO]\n");
    printf("  Range : [%.2f, %.2f]\n", min_v, max_v);
    printf("  Scale : %f\n", q_buf->scale_factor);
    printf("  Z-Point: %d\n", q_buf->zero_pt);

    // 4. Sikistirma dongusu
    for (i = 0; i < source->total_size; i++) {
        float real = source->mem.ptr_f[i];
        
        // Formul: q = (real / scale) + zero_point
        raw_q = (int32_t)round(real / q_buf->scale_factor) + q_buf->zero_pt;
        
        // CLAMP ile -128/127 araligina sabitle
        q_buf->mem.ptr_i[i] = (int8_t)CLAMP(raw_q, MIN_Q, MAX_Q);
    }

    return q_buf;
}

// Debug icin raporlama
void print_memory_stats(size_t size_f, size_t size_i) {
    printf("\n=== MEMORY OPTIMIZATION REPORT ===\n");
    printf("| Type      | Size (Byte)  | Status   |\n");
    printf("|-----------|--------------|----------|\n");
    printf("| Float32   | %-12zu | Original |\n", size_f);
    printf("| Int8      | %-12zu | Quantized|\n", size_i);
    printf("---------------------------------------\n");
    
    float ratio = (1.0f - ((float)size_i / (float)size_f)) * 100.0f;
    printf(">> Toplam Kazanc: %% %.1f \n\n", ratio);
}

// --- MAIN ---

int main() {
    int i;
    int matrix_shape[] = {5, 5}; // 5x5 Matrix
    size_t f_size, i_size;
    
    MicroBuffer* float_buf;
    MicroBuffer* int8_buf;

    srand(time(NULL)); 

    printf("TinyML Tensor Manager Baslatiliyor...\n");

    // Adim 1: Float Buffer Olustur
    float_buf = init_buffer(matrix_shape, 2, MODE_FLOAT32);
    
    // Random sensor verisi ata (-20 ile 70 arasi)
    printf(">> Veri simulasyonu yapiliyor...\n");
    for (i = 0; i < float_buf->total_size; i++) {
        float val = ((float)rand() / RAND_MAX) * 90.0f - 20.0f;
        float_buf->mem.ptr_f[i] = val;
    }

    printf("   Ornek Veri: %.2f, %.2f, %.2f ...\n", 
           float_buf->mem.ptr_f[0], float_buf->mem.ptr_f[1], float_buf->mem.ptr_f[2]);

    // Adim 2: Sikistirma (Quantization)
    int8_buf = compress_to_int8(float_buf);

    if (int8_buf) {
        printf("   Int8 Hali : %d, %d, %d ...\n", 
               int8_buf->mem.ptr_i[0], int8_buf->mem.ptr_i[1], int8_buf->mem.ptr_i[2]);
               
        // Adim 3: Raporla
        f_size = float_buf->total_size * sizeof(float);
        i_size = int8_buf->total_size * sizeof(int8_t);
        
        print_memory_stats(f_size, i_size);

        destroy_buffer(int8_buf);
    }

    destroy_buffer(float_buf);
    
    printf("Cikis icin Enter'a basin...");
    getchar();
    
    return 0;
}
