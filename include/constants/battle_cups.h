#ifndef GUARD_CONSTANTS_BATTLE_CUPS_H
#define GUARD_CONSTANTS_BATTLE_CUPS_H

// HACKROM: Stadium-style Battle Cups

#define BATTLE_CUP_POKE     0
#define BATTLE_CUP_PIKA     1
#define BATTLE_CUP_PETIT    2
#define BATTLE_CUP_PRIME    3
#define BATTLE_CUP_COUNT    4

// Ranks (Poké Cup + Prime Cup only)
#define BATTLE_CUP_RANK_POKE_BALL    0
#define BATTLE_CUP_RANK_GREAT_BALL   1
#define BATTLE_CUP_RANK_ULTRA_BALL   2
#define BATTLE_CUP_RANK_MASTER_BALL  3
#define BATTLE_CUP_RANK_COUNT        4

#define BATTLE_CUP_BATTLES_PER_RUN   8
#define BATTLE_CUP_PARTY_SIZE        3

// Challenge status (mirrors frontier-style)
#define BATTLE_CUP_STATUS_NONE       0
#define BATTLE_CUP_STATUS_ACTIVE     1
#define BATTLE_CUP_STATUS_WON        2
#define BATTLE_CUP_STATUS_LOST       3
#define BATTLE_CUP_STATUS_PAUSED     4

// Level / total rules
#define POKE_CUP_MIN_LEVEL           50
#define POKE_CUP_MAX_LEVEL           55
#define POKE_CUP_TOTAL_LEVEL         155

#define PIKA_CUP_MIN_LEVEL           15
#define PIKA_CUP_MAX_LEVEL           20
#define PIKA_CUP_TOTAL_LEVEL         50

#define PETIT_CUP_MIN_LEVEL          25
#define PETIT_CUP_MAX_LEVEL          30
#define PETIT_CUP_TOTAL_LEVEL        80
// height in dm (dex), weight in hg (dex): 2.0m = 20 dm, 20.0 kg = 200 hg
#define PETIT_CUP_MAX_HEIGHT_DM      20
#define PETIT_CUP_MAX_WEIGHT_HG      200

// Prime: no level limits

// Validation result codes (VAR_RESULT)
#define BATTLE_CUP_VALID_OK              0
#define BATTLE_CUP_VALID_FAIL_GENERIC    1  // generic message; NPC explains details

// Init challenge result
#define BATTLE_CUP_INIT_OK               0
#define BATTLE_CUP_INIT_INVALID_PARTY    1
#define BATTLE_CUP_INIT_NOT_UNLOCKED     2

#endif // GUARD_CONSTANTS_BATTLE_CUPS_H
