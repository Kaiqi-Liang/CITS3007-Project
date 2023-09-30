#ifndef P_AND_P_H
#define P_AND_P_H

#define DEFAULT_BUFFER_SIZE 512
#define MAX_ITEMS 10

#include <stddef.h>
#include <stdint.h>

/**
 * Stores the name and description for some item type
 * that can be carried by player characters as part of
 * their inventory.
 *
 * The 'name' and 'desc' fields must contain a valid,
 * NUL-terminated string of length at most DEFAULT_BUFFER_SIZE-1,
 * and should otherwise comply with the requirements
 * laid out in the project specification for valid
 * ItemDetails structs.
 */
struct ItemDetails {
	uint64_t itemID;
	char name[DEFAULT_BUFFER_SIZE];
	char desc[DEFAULT_BUFFER_SIZE];
};

/**
 * Used to record how many items of one particular
 * type are carried by a player character.
 */
struct ItemCarried {
	uint64_t itemID;
	size_t quantity;
};

/**
 * Used to record the social class of a character.
 */
enum CharacterSocialClass { MENDICANT, LABOURER, MERCHANT, GENTRY, ARISTOCRACY };

/**
 * Records the base details of a player character.
 * (Other structs are used to record additional details
 * required for the game.)
 *
 * The 'profession' and 'name' fields must each contain a valid, NUL-terminated
 * string of length at most DEFAULT_BUFFER_SIZE-1.
 *
 * The sum of all 'quantity' fields in the character's *inventory*
 * must not exceed MAX_ITEMS. (And since that means a character
 * can carry at most MAX_ITEMS distinct types of item, the length of the inventory
 * array is capped at that length.)
 *
 * The number of array elements currently containing valid
 * ItemCarried records is stored in the 'inventorySize' field.
 *
 * In all other respects, a Character struct and should comply with the
 * requirements laid out in the project specification for valid Character
 * structs.
 */
struct Character {
	uint64_t characterID;
	enum CharacterSocialClass socialClass;
	char profession[DEFAULT_BUFFER_SIZE];
	char name[DEFAULT_BUFFER_SIZE];
	size_t inventorySize;
	struct ItemCarried inventory[MAX_ITEMS];
};

int saveItemDetails(const struct ItemDetails *arr, size_t nmemb, int fd);

int saveItemDetailsToPath(const struct ItemDetails *arr, size_t nmemb, const char *filename);

int loadItemDetails(struct ItemDetails **ptr, size_t *nmemb, int fd);

int loadItemDetailsFromPath(struct ItemDetails **ptr, size_t *nmemb, const char *filename);

int isValidName(const char *str);

int isValidMultiword(const char *str);

int isValidItemDetails(const struct ItemDetails *id);

int isValidCharacter(const struct Character *c);

int saveCharacters(struct Character *arr, size_t nmemb, int fd);

int loadCharacters(struct Character **ptr, size_t *nmemb, int fd);

int secureLoad(const char *filepath);

void playGame(struct ItemDetails *ptr, size_t nmemb);

#endif
// P_AND_P_H
