#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <p_and_p.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

typedef ssize_t (*ioFuncPtr)(int, void *, size_t);  // can only be read(2) or write(2)
typedef struct ItemDetails ItemDetails;
typedef struct Character Character;
typedef struct ItemCarried ItemCarried;
static_assert(sizeof(size_t) == sizeof(uint64_t), "Assume size_t is 64 bit unsigned int");

static int processField(int fd, void *buf, size_t size, ioFuncPtr ioFunc);
static int processCharacter(Character *character, int fd, ioFuncPtr ioFunc);
static int isValidItemDetailsAll(const ItemDetails *arr, size_t nmemb);
static int isValidCharacters(const Character *arr, size_t nmemb);
static void sanitiseItemDetails(ItemDetails *arr, size_t nmemb);
static void sanitiseCharacters(Character *arr, size_t nmemb);
static void sanitiseBuffer(char *buffer);

/**
 * @brief Serialise an array of `ItemDetails` structs and store the array using
 * the `ItemDetails` file format
 */
int saveItemDetails(const ItemDetails *arr, size_t nmemb, int fd) {
	if (processField(fd, &nmemb, sizeof(nmemb), (ioFuncPtr)write) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	if (isValidItemDetailsAll(arr, nmemb) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	size_t size = sizeof(*arr) * nmemb;
	ItemDetails *copy = malloc(size);
	if (copy == NULL) {
		return EXIT_FAILURE;
	}
	sanitiseItemDetails(memcpy(copy, arr, size), nmemb);

	if (processField(fd, copy, size, (ioFuncPtr)write) == EXIT_FAILURE) {
		free(copy);
		return EXIT_FAILURE;
	}

	free(copy);
	return EXIT_SUCCESS;
}

/**
 * @brief Deseriaslise an array of `ItemDetails` structs from a file descriptor
 */
int loadItemDetails(ItemDetails **ptr, size_t *nmemb, int fd) {
	if (processField(fd, nmemb, sizeof(*nmemb), read) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	const size_t size = sizeof(ItemDetails) * *nmemb;
	*ptr = malloc(size);
	if (*ptr == NULL) {
		return EXIT_FAILURE;
	}

	if (processField(fd, *ptr, size, read) == EXIT_FAILURE) {
		goto err;
	}

	if (isValidItemDetailsAll(*ptr, *nmemb) == EXIT_FAILURE) {
		goto err;
	}

	sanitiseItemDetails(*ptr, *nmemb);
	return EXIT_SUCCESS;

err:  // make sure to free the allocated memory if any error occurs
	free(*ptr);
	return EXIT_FAILURE;
}

/**
 * @brief Check whether a string constitutes a valid `name` field
 */
int isValidName(const char *str) {
	size_t i = 0;
	for (; i < DEFAULT_BUFFER_SIZE; ++i) {
		if (i < DEFAULT_BUFFER_SIZE - 1) {
			if (str[i] == '\0') {
				break;
			} else if (!isgraph(str[i])) {
				return false;
			}
		} else if (str[i] != '\0') {
			return false;
		}
	}
	if (i == 0) {
		return false;
	}
	return true;
}

/**
 * @brief Check whether a string constitutes a valid `multi-word` field
 */
int isValidMultiword(const char *str) {
	size_t i = 0;
	for (; i < DEFAULT_BUFFER_SIZE; ++i) {
		if (i < DEFAULT_BUFFER_SIZE - 1) {
			if (str[i] == '\0') {
				break;
			} else if (str[i] == ' ') {
				if (i == 0) {
					return false;
				}
			} else if (!isgraph(str[i])) {
				return false;
			}
		} else if (str[i] != '\0') {
			return false;
		} else {
			break;
		}
	}
	if (i == 0 || str[i - 1] == ' ') {
		return false;
	}
	return true;
}

/**
 * @brief Check whether an `ItemDetails` struct is valid
 */
int isValidItemDetails(const ItemDetails *id) {
	return isValidName(id->name) && isValidMultiword(id->desc);
}

/**
 * @brief Check whether a `Character` struct is valid
 */
int isValidCharacter(const Character *c) {
	if (!isValidName(c->profession) || !isValidMultiword(c->name) || c->inventorySize > MAX_ITEMS) {
		return false;
	}
	size_t total_number_of_items = 0;
	for (size_t i = 0; i < c->inventorySize; ++i) {
		total_number_of_items += c->inventory[i].quantity;
	}
	return total_number_of_items <= MAX_ITEMS;
}

/**
 * @brief Serialise an array of `Character` structs and store the array using
 * the `Character` file format
 */
int saveCharacters(Character *arr, size_t nmemb, int fd) {
	if (processField(fd, &nmemb, sizeof(nmemb), (ioFuncPtr)write) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	if (isValidCharacters(arr, nmemb) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	sanitiseCharacters(arr, nmemb);

	for (size_t i = 0; i < nmemb; ++i) {
		processCharacter(&arr[i], fd, (ioFuncPtr)write);
	}

	return EXIT_SUCCESS;
}

/**
 * @brief Deseriaslise an array of `Character` structs from a file descriptor
 */
int loadCharacters(Character **ptr, size_t *nmemb, int fd) {
	if (processField(fd, nmemb, sizeof(*nmemb), read) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	const size_t size = sizeof(Character) * *nmemb;
	*ptr = malloc(size);
	if (*ptr == NULL) {
		return EXIT_FAILURE;
	}

	for (size_t i = 0; i < *nmemb; ++i) {
		if (processCharacter(&(*ptr)[i], fd, read) == EXIT_FAILURE) {
			goto err;
		}
	}

	if (isValidCharacters(*ptr, *nmemb) == EXIT_FAILURE) {
		goto err;
	}

	sanitiseCharacters(*ptr, *nmemb);
	return EXIT_SUCCESS;

err:  // make sure to free the allocated memory if any error occurs
	free(*ptr);
	return EXIT_FAILURE;
}

/**
 * @brief Acquire appropriate permissions, load the `ItemDetails` database, and
 * then permanently drop permissions
 */
int secureLoad(const char *filepath) {
	const struct passwd *pw = getpwnam("pitchpoltadmin");
	if (pw == NULL) {
		return 2;
	}

	if (seteuid(pw->pw_uid) == -1) {  // set the effective userID to the userID of pitchpoltadmin
		// the running process was launched from an executable that was not a setUID program
		// owned by pitchpoltadmin
		return 2;
	}

	const int fd = open(filepath, O_RDONLY);
	if (fd == -1) {
		return 1;
	}

	size_t nmemb;
	ItemDetails *item_details;
	if (loadItemDetails(&item_details, &nmemb, fd) == EXIT_FAILURE) {
		close(fd);
		return 1;
	}

	close(fd);
	const uid_t uid = getuid();
	if (setresuid(uid, uid, uid) == -1) {  // permanently dropping privileges
		return 2;
	}

	playGame(item_details, nmemb);
	return 0;
}

/**
 * @brief Read/write from/to a file to/from a buffer of a given size
 */
static int processField(int fd, void *buf, size_t size, ioFuncPtr ioFunc) {
	const ssize_t res = ioFunc(fd, buf, size);
	if (res == -1 || (size_t)res != size) {
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

/**
 * @brief Read/write each of the field in a `Character` struct from/to a file
 */
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

/**
 * @brief Check whether an array of `ItemDetails` structs are valid
 */
static int isValidItemDetailsAll(const ItemDetails *arr, size_t nmemb) {
	for (size_t i = 0; i < nmemb; ++i) {
		if (!isValidItemDetails(&arr[i])) {
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

/**
 * @brief Check whether an array of `Character` structs are valid
 */
static int isValidCharacters(const Character *arr, size_t nmemb) {
	for (size_t i = 0; i < nmemb; ++i) {
		if (!isValidCharacter(&arr[i])) {
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

/**
 * @brief Sanitise all the string fields in a `ItemDetails` struct
 */
static void sanitiseItemDetails(ItemDetails *arr, size_t nmemb) {
	for (size_t i = 0; i < nmemb; ++i) {
		sanitiseBuffer(arr[i].name);
		sanitiseBuffer(arr[i].desc);
	}
}

/**
 * @brief Sanitise all the string fields in a `ItemDetails` struct and the
 * `inventory` field
 */
static void sanitiseCharacters(Character *arr, size_t nmemb) {
	for (size_t i = 0; i < nmemb; ++i) {
		sanitiseBuffer(arr[i].name);
		sanitiseBuffer(arr[i].profession);
	}
}

/**
 * @brief Sanitise everything after the nul terminator
 */
static void sanitiseBuffer(char *buffer) {
	size_t length = strnlen(buffer, DEFAULT_BUFFER_SIZE);
	memset(buffer + length, 0, DEFAULT_BUFFER_SIZE - length);
}
