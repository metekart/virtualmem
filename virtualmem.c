
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#define FILE_PAGE_DATA_NAME		"data.txt"
#define FILE_PAGE_ACCESS_NAME	"reference.txt"

#define FILE_PAGE_ACS_DATA_LEN	100000
#define TOTAL_PAGE_LEN			52

#define ENTRY_HIT				1
#define ENTRY_MISS				0

#define PAGE_SIZE				2048
#define ENTRY_TABLE_SIZE		10

// Holds page data
typedef struct page {
	char data[PAGE_SIZE];
} page_t;

// Page frame table (page id with page)
typedef struct table {
	int page_id;
	page_t *page;
} table_t;

// Pages
page_t* pages = NULL;

// Number of pages
int num_of_pages = 0;

// Entry Table
table_t entry_table[ENTRY_TABLE_SIZE];

// Entry table size
int entry_table_size = 0;

// Page access list id's
int* page_access_list = NULL;

// Page access list size
int page_access_list_size = 0;

// Read file and fill data to pages with size of 2048 bytes
void read_pages_file(const char* filename)
{
	size_t len;
	char buffer[PAGE_SIZE];

	FILE* fp;

	fp = fopen(filename, "r");

	if (fp == NULL) {
		return;
	}

	while ((len = fread(buffer, sizeof(char), sizeof(buffer), fp)) > 0) {
		page_t *_pages = realloc(pages, sizeof(page_t) * (num_of_pages + 1));

		if (_pages != NULL) {
			pages = _pages;
			memcpy(pages[num_of_pages].data, buffer, len);
			num_of_pages++;
		}
	}

	fclose(fp);
}

// Read file page access structure
void read_page_access_file(const char* filename)
{
	int page_id;

	FILE* fp;

	fp = fopen(filename, "r");

	if (fp == NULL) {
		return;
	}

	while (fscanf(fp, "%d", &page_id) == 1) {
		int *_page_access_list = (int *) realloc(page_access_list, sizeof(int) * (page_access_list_size + 1));

		if (_page_access_list != NULL) {
			page_access_list = _page_access_list;
			page_access_list[page_access_list_size] = page_id;
			page_access_list_size++;
		}
	}

	fclose(fp);
}

// This function searches page_id in the entry table and set if not exist using FIFO 
// 0: Miss (Page Fault)
// 1: Hit (Found in the table)
// tlb_id is the page_id in the entry table
int find_set_table_fifo(int page_id, int* tlb_id)
{
	int i;
	page_t page;

	for (i = 0; i < entry_table_size; i++) {
		if (entry_table[i].page_id == page_id) {
			*tlb_id = i;
			return ENTRY_HIT;
		}
	}

	if (entry_table_size < ENTRY_TABLE_SIZE) {
		entry_table[entry_table_size].page_id = page_id;
		entry_table[entry_table_size].page = &pages[page_id];
		page = pages[page_id];
		page = page;
		*tlb_id = entry_table_size;
		entry_table_size++;
	}
	else {
		for (i = 0; i < ENTRY_TABLE_SIZE - 1; i++) {
			entry_table[i] = entry_table[i + 1];
		}

		entry_table[ENTRY_TABLE_SIZE - 1].page_id = page_id;
		entry_table[ENTRY_TABLE_SIZE - 1].page = &pages[page_id];
		page = pages[page_id];
		page = page;
		*tlb_id = ENTRY_TABLE_SIZE - 1;
	}

	return ENTRY_MISS;
}

// This function searches page_id in the entry table and set if not exist using LRU 
// 0: Miss (Page Fault)
// 1: Hit (Found in the table)
// tlb_id is the page_id in the entry table
int find_set_table_lru(int page_id, int* tlb_id)
{
	int i, j;
	page_t page;

	for (i = 0; i < entry_table_size; i++) {
		if (entry_table[i].page_id == page_id) {
			table_t tlb = entry_table[i];

			for (j = i; j < entry_table_size - 1; j++) {
				entry_table[j] = entry_table[j + 1];
			}

			entry_table[entry_table_size - 1] = tlb;
			*tlb_id = entry_table_size - 1;
			return ENTRY_HIT;
		}
	}

	if (entry_table_size < ENTRY_TABLE_SIZE) {
		entry_table[entry_table_size].page_id = page_id;
		entry_table[entry_table_size].page = &pages[page_id];
		page = pages[page_id];
		page = page;
		*tlb_id = entry_table_size;
		entry_table_size++;
	}
	else {
		for (i = 0; i < ENTRY_TABLE_SIZE - 1; i++) {
			entry_table[i] = entry_table[i + 1];
		}

		entry_table[ENTRY_TABLE_SIZE - 1].page_id = page_id;
		entry_table[ENTRY_TABLE_SIZE - 1].page = &pages[page_id];
		page = pages[page_id];
		page = page;
		*tlb_id = ENTRY_TABLE_SIZE - 1;
	}

	return ENTRY_MISS;
}

int find_nearest_access(int page_id, int offset)
{
	int i;

	for (i = offset; i < page_access_list_size; i++) {
		if (page_access_list[i] == page_id) {
			return i;
		}
	}

	return INT_MAX;
}

// This function searches page_id in the entry table and set if not exist using FIFO 
// 0: Miss (Page Fault)
// 1: Hit (Found in the table)
// tlb_id is the page_id in the entry table
int find_set_table_optimal(int page_id, int* tlb_id, int offset)
{
	int i;
	page_t page;

	for (i = 0; i < entry_table_size; i++) {
		if (entry_table[i].page_id == page_id) {
			*tlb_id = i;
			return ENTRY_HIT;
		}
	}

	if (entry_table_size < ENTRY_TABLE_SIZE) {
		entry_table[entry_table_size].page_id = page_id;
		entry_table[entry_table_size].page = &pages[page_id];
		page = pages[page_id];
		page = page;
		*tlb_id = entry_table_size;
		entry_table_size++;
	}
	else {
		int worst_page_id_index = -1;
		int worst_access = 0;

		for (i = 0; i < ENTRY_TABLE_SIZE; i++) {
			int access = find_nearest_access(entry_table[i].page_id, offset);

			if (access > worst_access) {
				worst_page_id_index = i;
				worst_access = access;

				if (worst_access == INT_MAX) {
					break;
				}
			}
		}

		for (i = worst_page_id_index; i < ENTRY_TABLE_SIZE - 1; i++) {
			entry_table[i] = entry_table[i + 1];
		}

		entry_table[ENTRY_TABLE_SIZE - 1].page_id = page_id;
		entry_table[ENTRY_TABLE_SIZE - 1].page = &pages[page_id];
		page = pages[page_id];
		page = page;
		*tlb_id = ENTRY_TABLE_SIZE - 1;
	}

	return ENTRY_MISS;
}

void create_page_file()
{
	int i;
	char buf[PAGE_SIZE];

	FILE* fp;
	
	fp = fopen(FILE_PAGE_DATA_NAME, "w");

	if (fp == NULL) {
		return;
	}

	for (i = 0; i < TOTAL_PAGE_LEN / 2; i++) {
		memset(buf, 'A' + i, sizeof(buf));
		fwrite(buf, sizeof(char), sizeof(buf), fp);
	}

	for (i = 0; i < TOTAL_PAGE_LEN / 2; i++) {
		memset(buf, 'a' + i, sizeof(buf));
		fwrite(buf, sizeof(char), sizeof(buf), fp);
	}

	fclose(fp);
}

void create_reference_string_file()
{
	int i;
	int walk;
	double r;

	FILE* fp;

	srand((unsigned int) time(NULL));

	fp = fopen(FILE_PAGE_ACCESS_NAME, "w");

	if (fp == NULL) {
		return;
	}

	walk = 0;

	for (i = 0; i < FILE_PAGE_ACS_DATA_LEN; i++) {
		fprintf(fp, "%d\n", walk);

		r = (double) rand() / RAND_MAX;

		if (r <= 0.33) {
			walk = (walk + 1) % TOTAL_PAGE_LEN;
		}
		else if (r <= 0.66) {
			walk = (walk - 1 + TOTAL_PAGE_LEN) % TOTAL_PAGE_LEN;
		}
		else {
			walk = walk;
		}
	}

	fclose(fp);
}

void init_entry_table()
{
	int i;

	for (i = 0; i < ENTRY_TABLE_SIZE; i++) {
		entry_table->page_id = 0;
		entry_table->page = NULL;
	}

	entry_table_size = 0;
}

void run_fifo()
{
	int i, res;
	int tbl_id;
	int total_miss = 0;

	clock_t start, end;
	double cpu_time_used;

	init_entry_table();

	start = clock();

	for (i = 0; i < page_access_list_size; i++) {
		res = find_set_table_fifo(page_access_list[i], &tbl_id);

		if (res == ENTRY_MISS)
			total_miss++;
	}

	end = clock();
	cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;

	printf("FIFO Total miss: %d\n", total_miss);
	printf("FIFO Time elapsed: %lf sec\n", cpu_time_used);
}

void run_lru()
{
	int i, res;
	int tbl_id;
	int total_miss = 0;

	clock_t start, end;
	double cpu_time_used;

	init_entry_table();

	start = clock();

	for (i = 0; i < page_access_list_size; i++) {
		res = find_set_table_lru(page_access_list[i], &tbl_id);

		if (res == ENTRY_MISS)
			total_miss++;
	}

	end = clock();
	cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;

	printf("LRU Total miss: %d\n", total_miss);
	printf("LRU Time elapsed: %lf sec\n", cpu_time_used);
}

void run_optimal()
{
	int i, res;
	int tbl_id;
	int total_miss = 0;

	clock_t start, end;
	double cpu_time_used;

	init_entry_table();

	start = clock();
	
	for (i = 0; i < page_access_list_size; i++) {
		res = find_set_table_optimal(page_access_list[i], &tbl_id, i + 1);

		if (res == ENTRY_MISS)
			total_miss++;
	}

	end = clock();
	cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;

	printf("Optimal Total miss: %d\n", total_miss);
	printf("Optimal Time elapsed: %lf sec\n", cpu_time_used);
}

void usage(const char* name)
{
	printf("Usage: %s           -> runs the simulation\n", name);
	printf("Usage: %s create    -> creates data and reference file\n", name);
	printf("Usage: %s -h        -> print usage\n", name);
	printf("Usage: %s --help    -> print usage\n", name);
}

int main(int argc, char* argv[])
{
	int page_id;

	if (argc > 1) {
		char* command;

		command = argv[1];

		if (strcmp(command, "create") == 0) {
			create_page_file();
			create_reference_string_file();

			return 0;
		}
		else if (strcmp(command, "-h") == 0 || strcmp(command, "--help") == 0) {
			usage(argv[0]);

			return 0;
		}
		else {
			usage(argv[0]);

			return -1;
		}
	}

	read_pages_file(FILE_PAGE_DATA_NAME);
	read_page_access_file(FILE_PAGE_ACCESS_NAME);

	do {
		printf("Enter page: ");
		scanf("%d", &page_id);

		if (page_id >= 0 && page_id < TOTAL_PAGE_LEN) {
			char buf[PAGE_SIZE + 1];

			memcpy(buf, pages[page_id].data, sizeof(pages[page_id].data));
			buf[PAGE_SIZE] = '\0';

			printf("%s\n", buf);
		}
	} while (page_id != -1);
	
	run_fifo();
	run_lru();
	run_optimal();

	return 0;
}
