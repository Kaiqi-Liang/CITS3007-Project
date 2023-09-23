#include <assert.h>
#include <p_and_p.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct ItemDetails ItemDetails;
static_assert(sizeof(size_t) == sizeof(uint64_t), "Assume size_t is 64 bits");

int saveItemDetails(const ItemDetails *arr, size_t numEls, int fd) {
	ssize_t res;
	if ((res = write(fd, &numEls, sizeof(size_t)) == -1) ||
	    (size_t)res != sizeof(size_t))
	{
		perror("Failed to write number");
		return EXIT_FAILURE;
	}
	if ((res = write(fd, arr, sizeof(ItemDetails) * numEls) == -1) ||
	    (size_t)res != sizeof(ItemDetails) * numEls)
	{
		perror("Failed to write structs");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int saveItemDetailsToPath(
    const ItemDetails *arr,
    size_t numEls,
    const char *filename
) {
	return 0;
}

int loadItemDetails(ItemDetails **ptr, size_t *numEls, int fd) {
	ssize_t res;
	if ((res = read(fd, numEls, sizeof(size_t)) == -1) ||
	    (size_t)res != sizeof(size_t))
	{
		perror("Failed to read number");
		return EXIT_FAILURE;
	}
	const size_t size = sizeof(ItemDetails) * *numEls;
	*ptr = malloc(size);
	if ((res = read(fd, *ptr, size) == -1) || (size_t)res != size) {
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int isValidName(const char *str) {
	return 0;
}

int isValidMultiword(const char *str) {
	return 0;
}

int isValidItemDetails(const ItemDetails *id) {
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

void playGame(ItemDetails *ptr, size_t numEls);
