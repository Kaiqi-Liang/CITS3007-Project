#include <p_and_p.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int saveItemDetails(const struct ItemDetails *arr, size_t numEls, int fd) {
	return 0;
}

int saveItemDetailsToPath(
    const struct ItemDetails *arr,
    size_t numEls,
    const char *filename
) {
	return 0;
}

int loadItemDetails(struct ItemDetails **ptr, size_t *numEls, int fd) {
	return 0;
}

int isValidName(const char *str) {
	return 0;
}

int isValidMultiword(const char *str) {
	return 0;
}

int isValidItemDetails(const struct ItemDetails *id) {
	return 0;
}

int isValidCharacter(const struct Character *c) {
	return 0;
}

int saveCharacters(struct Character *arr, size_t numEls, int fd) {
	return 0;
}

int loadCharacters(struct Character **ptr, size_t *numEls, int fd) {
	return 0;
}

int secureLoad(const char *filepath) {
	return 0;
}

void playGame(struct ItemDetails *ptr, size_t numEls);
