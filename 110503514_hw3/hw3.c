#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

//include the two algorithm
#include "arcd/arcd/arcd.h"
#include "huffman/huffman.h"

//other for arcd_freg
#include "adaptive_model.h"
typedef unsigned char symbol_t;
static const arcd_char_t EOS = 1 << (8 * sizeof(symbol_t));


//#define total_time
#include <string.h>


void output(const arcd_buf_t buf, const unsigned buf_bits, void *const io);
unsigned input(arcd_buf_t *const buf, void *const io);

int main(int argc, char *argv[]){
    FILE * IN; // same input file
    FILE * HOUT, *HTMP; // huffman output file
    FILE * AOUT, *ATMP; // ardc output file

    if(argc != 2){
        printf("Please input the input file name.\n");
        return 1;
    }
    // Read an input file in binary format
    if(!(IN = fopen(argv[1], "rb"))){
        printf("Error file name.\n");
        return 1;
    }
    
    //Write/Read an input file in binary format
    //store encoding result
    HTMP = fopen("h_tmp", "wb+"); 
    ATMP = fopen("a_tmp", "wb+"); 

    //Write an input file in binary format
    //store decoding result
    HOUT = fopen("h_output", "wb"); 
    AOUT = fopen("a_output", "wb"); 

    //time variable
    double time_spent;
    clock_t begin, end;

    //file variable
    struct stat in, h_compress, a_compress;

    //whether succeed
    int rc = -1;
    
    //----------------Test huffman----------------
    printf("----------------Test huffman----------------\n");
    
    //----------------encode time----------------
    begin = clock();
    rc = huffman_encode_file(IN, HTMP); // the result will store in HTMP
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("huffman encode = %lf sec\n", time_spent);

    //----------------decode time----------------
    fseek(IN, SEEK_SET, 0);
    begin = clock();
    rc = huffman_decode_file(HTMP, HOUT); // read encoded code and decode it
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("huffman decode = %lf sec\n", time_spent);
    
    //----------------Test ardc----------------
    printf("----------------Test ardc----------------\n");
    fseek(IN, SEEK_SET, 0);
    adaptive_model model_in, model_out;
    adaptive_model_create(&model_in, EOS + 1);
    adaptive_model_create(&model_out, EOS + 1);
    //----------------encode time----------------
    begin = clock();
	arcd_enc enc;
	arcd_enc_init(&enc, adaptive_model_getprob, &model_in, output, ATMP);
	symbol_t sym;

    clock_t b, e;
	while (0 < fread(&sym, sizeof(sym), 1, IN))
	{   
		arcd_enc_put(&enc, sym);  // the result will store in ATMP
	}
	arcd_enc_put(&enc, EOS);
	arcd_enc_fin(&enc);
	
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("ardc encode = %lf sec\n", time_spent);

    //----------------decode time----------------
    begin = clock();
    arcd_dec dec;
    fseek(ATMP, SEEK_SET, 0);
	arcd_dec_init(&dec, adaptive_model_getch, &model_out, input , ATMP);
	arcd_char_t ch;
	while (EOS != (ch = arcd_dec_get(&dec))/*read encoded code and decode it*/)
	{
        const symbol_t sym = (unsigned char)ch;
		fwrite(&sym, sizeof(sym), 1, AOUT);
	}
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("ardc decode = %lf sec\n", time_spent);
    
    //free memory allocation
    adaptive_model_free(&model_in);
    adaptive_model_free(&model_out);

    //get compress rate
    printf("----------------Compress file size----------------\n");
    stat(argv[1], &in);
    stat("h_tmp", &h_compress);
    stat("a_tmp", &a_compress);

    printf("Input: %lu bytes \n", in.st_size);
    printf("Huff_compress: %lu bytes\n", h_compress.st_size);
    printf("Ardc_compress: %lu bytes\n", a_compress.st_size);


    //close all file
    fclose(IN);
    fclose(HTMP);
    fclose(ATMP);
    fclose(HOUT);
    fclose(AOUT);
    return 0;
}

void output(const arcd_buf_t buf, const unsigned buf_bits, void *const io)
{
	(void)buf_bits;
	FILE *const f = (FILE *)io;
	fwrite(&buf, sizeof(buf), 1, f);
}

unsigned input(arcd_buf_t *const buf, void *const io)
{
	FILE *const f = (FILE *)io;
	return 8 * fread(buf, sizeof(*buf), 1, f);
}
