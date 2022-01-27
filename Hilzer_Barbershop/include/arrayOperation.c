#include "arrayOperation.h"

void printArray(int* v, int n, char* str) {
	printf("Stampo l'array: %s\n", str);
	for (int i=0;i<n;i++) {
		printf("%d:\t %d\n", i, v[i]);
	}
}

int getFirstFreePosition(int* v, int n) {
	for (int i=0;i < n; i++) {
		if (v[i] == -1) {
			return i;
		}
	}
	return -1;
}

int getFirstNonFreePosition(int* v, int n) {
	for (int i=0;i < n; i++) {
		if (v[i] != -1) {
			return i;
		}
	}
	return -1;
}

int getIndex(int* v, int n, int val) {
	for (int i=0;i < n; i++) {
		if (v[i] == val) {
			return i;
		}
		if (v[i] == -1) {
			break;
		}
	}
	return -1;
}