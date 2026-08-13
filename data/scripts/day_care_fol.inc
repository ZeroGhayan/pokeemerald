@ ============================================================
@ HACKROM: Day-Care NPCs (Old Lady / Breeder / Lass)
@ Inclua este ficheiro a partir do scripts.inc do mapa do Day-Care.
@ Ajuste localIds, flags e textos à vontade.
@ ============================================================

@ VAR_RESULT codes (StoreSelectedPokemonInDaycare)
@ 0 = ok, 1 = full, 2 = not female

@ Daycare_BreederCheck codes
@ 0 NOTHING, 1 EGG_FOUND, 2 NO_MON, 3 ALREADY_CHECKED
@ 4 EGG_WAITING, 5 CONDOM_BLOCKED

@ StartDaycareBattle / after battle
@ before: 2 = no mon, 3 = no battles left today
@ after (CB2): 0 = normal, 1 = condom broke this battle

@ ------------------------------------------------------------
@ A) OLD LADY — deposit / withdraw (female only, 1 slot)
@ ------------------------------------------------------------
EventScript_DaycareOldLady::
	lock
	faceplayer
	specialvar VAR_RESULT, Daycare_HasMon
	compare VAR_RESULT, TRUE
	goto_if_eq EventScript_DaycareOldLady_HasMon
	@ No mon deposited
	msgbox Daycare_Text_OldLady_Welcome, MSGBOX_YESNO
	compare VAR_RESULT, NO
	goto_if_eq EventScript_DaycareOldLady_Bye
	special ChooseSendDaycareMon
	waitstate
	compare VAR_0x8004, PARTY_SIZE
	goto_if_eq EventScript_DaycareOldLady_Bye
	special StoreSelectedPokemonInDaycare
	compare VAR_RESULT, 1
	goto_if_eq EventScript_DaycareOldLady_Full
	compare VAR_RESULT, 2
	goto_if_eq EventScript_DaycareOldLady_NotFemale
	msgbox Daycare_Text_OldLady_Deposited, MSGBOX_DEFAULT
	release
	end

EventScript_DaycareOldLady_HasMon::
	msgbox Daycare_Text_OldLady_TakeBack, MSGBOX_YESNO
	compare VAR_RESULT, NO
	goto_if_eq EventScript_DaycareOldLady_Bye
	@ Cost optional — use GetDaycareCostAndPrepareString if desired
	special TakePokemonFromDaycare
	msgbox Daycare_Text_OldLady_Returned, MSGBOX_DEFAULT
	release
	end

EventScript_DaycareOldLady_Full::
	msgbox Daycare_Text_OldLady_Full, MSGBOX_DEFAULT
	release
	end

EventScript_DaycareOldLady_NotFemale::
	msgbox Daycare_Text_OldLady_OnlyFemale, MSGBOX_DEFAULT
	release
	end

EventScript_DaycareOldLady_Bye::
	msgbox Daycare_Text_OldLady_Bye, MSGBOX_DEFAULT
	release
	end

@ ------------------------------------------------------------
@ B) BREEDER — daily check (priority: pending egg)
@ ------------------------------------------------------------
EventScript_DaycareBreeder::
	lock
	faceplayer
	specialvar VAR_RESULT, Daycare_BreederCheck
	switch VAR_RESULT
	case 4, EventScript_DaycareBreeder_EggWaiting
	case 1, EventScript_DaycareBreeder_EggFound
	case 2, EventScript_DaycareBreeder_NoMon
	case 3, EventScript_DaycareBreeder_Already
	case 5, EventScript_DaycareBreeder_Condom
	case 0, EventScript_DaycareBreeder_Nothing
	release
	end

EventScript_DaycareBreeder_EggWaiting::
	msgbox Daycare_Text_Breeder_EggWaiting, MSGBOX_DEFAULT
	goto EventScript_DaycareBreeder_TryGiveEgg

EventScript_DaycareBreeder_EggFound::
	msgbox Daycare_Text_Breeder_EggFound, MSGBOX_DEFAULT
	goto EventScript_DaycareBreeder_TryGiveEgg

EventScript_DaycareBreeder_TryGiveEgg::
	getpartysize
	compare VAR_RESULT, PARTY_SIZE
	goto_if_eq EventScript_DaycareBreeder_PartyFull
	special GiveEggFromDaycare
	msgbox Daycare_Text_Breeder_ReceivedEgg, MSGBOX_DEFAULT
	release
	end

EventScript_DaycareBreeder_PartyFull::
	msgbox Daycare_Text_Breeder_PartyFull, MSGBOX_DEFAULT
	release
	end

EventScript_DaycareBreeder_NoMon::
	msgbox Daycare_Text_Breeder_NoMon, MSGBOX_DEFAULT
	release
	end

EventScript_DaycareBreeder_Already::
	msgbox Daycare_Text_Breeder_Already, MSGBOX_DEFAULT
	release
	end

EventScript_DaycareBreeder_Condom::
	msgbox Daycare_Text_Breeder_Condom, MSGBOX_DEFAULT
	release
	end

EventScript_DaycareBreeder_Nothing::
	msgbox Daycare_Text_Breeder_Nothing, MSGBOX_DEFAULT
	release
	end

@ ------------------------------------------------------------
@ C) LASS — up to 3 play battles / day
@ ------------------------------------------------------------
EventScript_DaycareLass::
	lock
	faceplayer
	specialvar VAR_RESULT, Daycare_HasMon
	compare VAR_RESULT, FALSE
	goto_if_eq EventScript_DaycareLass_NoMon
	specialvar VAR_RESULT, Daycare_CanBattleToday
	compare VAR_RESULT, FALSE
	goto_if_eq EventScript_DaycareLass_NoBattles
	msgbox Daycare_Text_Lass_AskBattle, MSGBOX_YESNO
	compare VAR_RESULT, NO
	goto_if_eq EventScript_DaycareLass_Bye
	msgbox Daycare_Text_Lass_LetsGo, MSGBOX_DEFAULT
	special StartDaycareBattle
	waitstate
	@ After battle: VAR_RESULT 1 = condom broke this session
	compare VAR_RESULT, 1
	goto_if_eq EventScript_DaycareLass_CondomBroke
	msgbox Daycare_Text_Lass_AfterBattle, MSGBOX_DEFAULT
	release
	end

EventScript_DaycareLass_CondomBroke::
	msgbox Daycare_Text_Lass_CondomBroke, MSGBOX_DEFAULT
	release
	end

EventScript_DaycareLass_NoMon::
	msgbox Daycare_Text_Lass_NoMon, MSGBOX_DEFAULT
	release
	end

EventScript_DaycareLass_NoBattles::
	msgbox Daycare_Text_Lass_NoBattles, MSGBOX_DEFAULT
	release
	end

EventScript_DaycareLass_Bye::
	msgbox Daycare_Text_Lass_Bye, MSGBOX_DEFAULT
	release
	end

@ ------------------------------------------------------------
@ Texts (placeholder — rewrite freely)
@ ------------------------------------------------------------
Daycare_Text_OldLady_Welcome:
	.string "Want me to look after a female\n"
	.string "POKéMON for a while?$"

Daycare_Text_OldLady_Deposited:
	.string "All right, I'll take care of it.\n"
	.string "Come back anytime.$"

Daycare_Text_OldLady_TakeBack:
	.string "Your POKéMON is doing fine.\n"
	.string "Do you want it back?$"

Daycare_Text_OldLady_Returned:
	.string "Here you go. Take good care of it.$"

Daycare_Text_OldLady_Full:
	.string "I can only look after one POKéMON.$"

Daycare_Text_OldLady_OnlyFemale:
	.string "Oh, dear… I only take female\n"
	.string "POKéMON here.$"

Daycare_Text_OldLady_Bye:
	.string "Come back if you need me.$"

Daycare_Text_Breeder_EggWaiting:
	.string "The EGG is still waiting for you!$"

Daycare_Text_Breeder_EggFound:
	.string "Good news!\n"
	.string "Your POKéMON had an EGG!$"

Daycare_Text_Breeder_ReceivedEgg:
	.string "{PLAYER} received the EGG from\n"
	.string "the BREEDER.$"

Daycare_Text_Breeder_PartyFull:
	.string "Your party is full…\n"
	.string "I'll hold the EGG until you return.$"

Daycare_Text_Breeder_NoMon:
	.string "There's no POKéMON here right now.$"

Daycare_Text_Breeder_Already:
	.string "I already checked today.\n"
	.string "Come back tomorrow.$"

Daycare_Text_Breeder_Condom:
	.string "Hmm… nothing will happen while\n"
	.string "that CONDOM is still on.$"

Daycare_Text_Breeder_Nothing:
	.string "I checked carefully, but nothing\n"
	.string "changed. Tomorrow, maybe.$"

Daycare_Text_Lass_AskBattle:
	.string "Want to play with the POKéMON\n"
	.string "staying here?$"

Daycare_Text_Lass_LetsGo:
	.string "Okay! Come inside…$"

Daycare_Text_Lass_AfterBattle:
	.string "That was fun!\n"
	.string "Your POKéMON looks happier.$"

Daycare_Text_Lass_CondomBroke:
	.string "Oh my… the CONDOM broke!\p"
	.string "Things might get more… interesting\n"
	.string "around here now.$"

Daycare_Text_Lass_NoMon:
	.string "There's no POKéMON to play with.$"

Daycare_Text_Lass_NoBattles:
	.string "That's enough for today.\n"
	.string "Come back tomorrow!$"

Daycare_Text_Lass_Bye:
	.string "See you!$"