#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <p_and_p.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

typedef ssize_t (*ioFuncPtr)(int, void *, size_t); // can only be read(2) or write(2)
typedef struct ItemDetails ItemDetails;
typedef struct Character Character;
typedef struct ItemCarried ItemCarried;
static_assert(sizeof(size_t) == sizeof(uint64_t), "Assume size_t is 64 bit unsigned int");

static int processField(int fd, void *buf, size_t size, ioFuncPtr ioFunc);
static int processCharacter(Character *character, int fd, ioFuncPtr ioFunc);
static int isValidItemDetailsAll(const ItemDetails *arr, size_t numEls);
static int isValidCharacters(const Character *arr, size_t numEls);
static void sanitiseItemDetails(ItemDetails *arr, size_t numEls);
static void sanitiseCharacters(Character *arr, size_t numEls);
static void sanitiseString(char *buffer);
static void sanitiseBuffer(char *buffer, size_t valid_length, size_t max_size);

int saveItemDetails(ItemDetails *arr, size_t numEls, int fd) {
	if (processField(fd, &numEls, sizeof(numEls), (ioFuncPtr)write) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	if (isValidItemDetailsAll(arr, numEls) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	sanitiseItemDetails(arr, numEls);

	if (processField(fd, arr, sizeof(*arr) * numEls, (ioFuncPtr)write) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int loadItemDetails(ItemDetails **ptr, size_t *numEls, int fd) {
	if (processField(fd, numEls, sizeof(*numEls), read) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	const size_t size = sizeof(ItemDetails) * *numEls;
	*ptr = malloc(size);
	if (ptr == NULL) {
		return EXIT_FAILURE;
	}

	if (processField(fd, *ptr, size, read) == EXIT_FAILURE) {
		goto err;
	}

	if (isValidItemDetailsAll(*ptr, *numEls) == EXIT_FAILURE) {
		goto err;
	}

	sanitiseItemDetails(*ptr, *numEls);
	return EXIT_SUCCESS;

err: // make sure to free the allocated memory if any error occurs
	free(*ptr);
	return EXIT_FAILURE;
}

int isValidName(const char *str) {
	for (size_t i = 0; i < DEFAULT_BUFFER_SIZE; i++) {
		if (i < DEFAULT_BUFFER_SIZE - 1) {
			if (str[i] == '\0') {
				return EXIT_SUCCESS;
			} else if (!isgraph(str[i])) {
				return EXIT_FAILURE;
			}
		} else if (str[i] != '\0') {
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

int isValidMultiword(const char *str) {
	size_t i = 0;
	for (; i < DEFAULT_BUFFER_SIZE; i++) {
		if (i < DEFAULT_BUFFER_SIZE - 1) {
			if (str[i] == '\0') {
				break;
			} else if (str[i] == ' ') {
				if (i == 0) {
					return EXIT_FAILURE;
				}
			} else if (!isgraph(str[i])) {
				return EXIT_FAILURE;
			}
		} else if (str[i] != '\0') {
			return EXIT_FAILURE;
		} else {
			break;
		}
	}
	if (i > 0 && str[i - 1] == ' ') {
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int isValidItemDetails(const ItemDetails *id) {
	if (isValidName(id->name) == EXIT_SUCCESS && isValidMultiword(id->desc) == EXIT_SUCCESS) {
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}

int isValidCharacter(const Character *c) {
	if (isValidName(c->profession) == EXIT_SUCCESS && isValidMultiword(c->name) == EXIT_SUCCESS &&
	    c->inventorySize <= MAX_ITEMS)
	{
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}

int saveCharacters(Character *arr, size_t numEls, int fd) {
	if (processField(fd, &numEls, sizeof(numEls), (ioFuncPtr)write) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	if (isValidCharacters(arr, numEls) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	sanitiseCharacters(arr, numEls);

	for (size_t i = 0; i < numEls; ++i) {
		processCharacter(&arr[i], fd, (ioFuncPtr)write);
	}

	return EXIT_SUCCESS;
}

int loadCharacters(Character **ptr, size_t *numEls, int fd) {
	if (processField(fd, numEls, sizeof(*numEls), read) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	const size_t size = sizeof(Character) * *numEls;
	*ptr = malloc(size);
	if (ptr == NULL) {
		return EXIT_FAILURE;
	}

	for (size_t i = 0; i < *numEls; ++i) {
		if (processCharacter(&(*ptr)[i], fd, read) == EXIT_FAILURE) {
			goto err;
		}
	}

	if (isValidCharacters(*ptr, *numEls) == EXIT_FAILURE) {
		goto err;
	}

	sanitiseCharacters(*ptr, *numEls);
	return EXIT_SUCCESS;

err: // make sure to free the allocated memory if any error occurs
	free(*ptr);
	return EXIT_FAILURE;
}

int secureLoad(const char *filepath) {
	const int fd = open(filepath, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
	size_t numEls;
	ItemDetails *item_details;
	if (loadItemDetails(&item_details, &numEls, fd) == EXIT_FAILURE) {
		return 1;
	}
	if (setuid(getuid()) == -1) {
		return 2;
	}
	playGame(item_details, numEls);
	return 0;
}

void playGame(struct ItemDetails *ptr, size_t numEls) {}

static int processField(int fd, void *buf, size_t size, ioFuncPtr ioFunc) {
	const ssize_t res = ioFunc(fd, buf, size);
	if (res == -1 || (size_t)res != size) {
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

static int processCharacter(Character *character, int fd, ioFuncPtr ioFunc) {
	if (processField(fd, &character->characterID, sizeof(character->characterID), ioFunc) ==
	    EXIT_FAILURE)
	{
		return EXIT_FAILURE;
	}

	if (processField(fd, &character->socialClass, sizeof(character->socialClass), ioFunc) ==
	    EXIT_FAILURE)
	{
		return EXIT_FAILURE;
	}

	if (processField(fd, character->profession, sizeof(character->profession), ioFunc) ==
	    EXIT_FAILURE)
	{
		return EXIT_FAILURE;
	}

	if (processField(fd, character->name, sizeof(character->name), ioFunc) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	if (processField(fd, &character->inventorySize, sizeof(character->inventorySize), ioFunc) ==
	    EXIT_FAILURE)
	{
		return EXIT_FAILURE;
	}

	if (processField(
	        fd,
	        character->inventory,
	        sizeof(ItemCarried) * character->inventorySize,
	        ioFunc
	    ) == EXIT_FAILURE)
	{
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static int isValidItemDetailsAll(const ItemDetails *arr, size_t numEls) {
	for (size_t i = 0; i < numEls; i++) {
		if (isValidItemDetails(&arr[i]) == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

static int isValidCharacters(const Character *arr, size_t numEls) {
	for (size_t i = 0; i < numEls; i++) {
		if (isValidCharacter(&arr[i]) == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

static void sanitiseItemDetails(ItemDetails *arr, size_t numEls) {
	for (size_t i = 0; i < numEls; i++) {
		sanitiseString(arr[i].name);
		sanitiseString(arr[i].desc);
	}
}

static void sanitiseCharacters(Character *arr, size_t numEls) {
	for (size_t i = 0; i < numEls; i++) {
		sanitiseString(arr[i].name);
		sanitiseString(arr[i].profession);
		sanitiseBuffer(
		    arr[i].profession,
		    sizeof(ItemCarried) * arr[i].inventorySize,
		    sizeof(ItemCarried) * MAX_ITEMS
		);
	}
}
static void sanitiseString(char *buffer) {
	size_t length = strlen(buffer);
	sanitiseBuffer(buffer, length, DEFAULT_BUFFER_SIZE);
}

static void sanitiseBuffer(char *buffer, size_t valid_length, size_t max_size) {
	memset(buffer + valid_length, '\0', max_size - valid_length);
}
