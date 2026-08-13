@ After Special_BattleCupOnBattleWon returns STATUS_WON:
@   special Special_BattleCupGivePrize
@   @ R1: key item to bag (VAR_RESULT 1 = bag full)
@   @ R2: gift mon to party/PC (VAR_RESULT 1 = both full)
@
@ Trainer Card symbols: read VAR 0x40FD bitfield
@   bits 0-3: R1 champ POKE/PIKA/PETIT/PRIME
@   bits 4-7: R2 cleared
@   bits 8-10 / 11-13: rank unlocks
@ Graphics on Trainer Card = later step (draw icons from these bits).
