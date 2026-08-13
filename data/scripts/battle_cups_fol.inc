@ HACKROM: Battle Cup challenge loop (skeleton for map scripts)
@
@ Before start:
@   setvar VAR_BATTLE_CUP_ID, BATTLE_CUP_*
@   setvar VAR_BATTLE_CUP_RANK, BATTLE_CUP_RANK_*   @ 0 if Pika/Petit
@   setvar VAR_BATTLE_CUP_IS_R2, 0 or 1
@   special Special_BattleCupInit
@   @ VAR_RESULT: 0 ok, 1 invalid party, 2 not unlocked
@
@ Choose 3:
@   @ fill VAR_0x8000/01/02 with party indices (0-based)
@   special Special_BattleCupSetSelectedFromVars
@   @ VAR_RESULT: 0 ok else fail
@   special Special_BattleCupApplySelectedParty
@
@ Each battle 0-7:
@   special Special_BattleCupIsFinal
@   @ if TRUE -> fixed final trainer (step 5); else random pool (step 4)
@   @ ... start trainer battle ...
@   @ on win: setvar VAR_0x8000, 1 if perfect else 0
@   special Special_BattleCupOnBattleWon
@   @ VAR_RESULT == BATTLE_CUP_STATUS_WON -> finished run, give prize
@   @ on lose:
@   special Special_BattleCupOnBattleLost
@   @ VAR_RESULT TRUE -> can rematch
@   special Special_BattleCupTryRematch
@
@ After run (win or lose):
@   special Special_BattleCupRestoreParty
