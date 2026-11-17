@ HACKROM (Forest of Love): subsolo (B1F) compartilhado por todos os Centros
@ Pokémon. Simulador de batalhas (regras da Battle Tower, sem Battle Frontier)
@ e as 4 lojas de BP: decorações, itens e os 2 tutores de golpes.
@ A volta para o 1F certo é feita pelo warp dinâmico (ver src/fol_pc_b1f.c).

PokemonCenter_B1F_MapScripts::
	.byte 0

@ -----------------------------------------------------------------------------
@ Simulador de batalhas (mecânica da Battle Tower)
@ -----------------------------------------------------------------------------
PokemonCenter_B1F_EventScript_SimulatorAttendant::
	lock
	faceplayer
	setvar VAR_FRONTIER_FACILITY, FRONTIER_FACILITY_TOWER
	special SavePlayerParty
	msgbox PokemonCenter_B1F_Text_SimWelcome, MSGBOX_DEFAULT
PokemonCenter_B1F_EventScript_SimAskMode::
	message PokemonCenter_B1F_Text_SimWhichMode
	waitmessage
	multichoice 17, 4, MULTI_FOL_SIM_MODE, FALSE
	switch VAR_RESULT
	case 0, PokemonCenter_B1F_EventScript_SimSingles
	case 1, PokemonCenter_B1F_EventScript_SimDoubles
	case 2, PokemonCenter_B1F_EventScript_SimInfo
	goto PokemonCenter_B1F_EventScript_SimCancel
	end

PokemonCenter_B1F_EventScript_SimInfo::
	msgbox PokemonCenter_B1F_Text_SimInfo, MSGBOX_DEFAULT
	goto PokemonCenter_B1F_EventScript_SimAskMode
	end

PokemonCenter_B1F_EventScript_SimSingles::
	setvar VAR_FRONTIER_BATTLE_MODE, FRONTIER_MODE_SINGLES
	goto PokemonCenter_B1F_EventScript_SimAskLevel
	end

PokemonCenter_B1F_EventScript_SimDoubles::
	setvar VAR_FRONTIER_BATTLE_MODE, FRONTIER_MODE_DOUBLES
	goto PokemonCenter_B1F_EventScript_SimAskLevel
	end

PokemonCenter_B1F_EventScript_SimAskLevel::
	message BattleFrontier_BattleTowerLobby_Text_WhichLevelMode
	waitmessage
	multichoice 17, 6, MULTI_LEVEL_MODE, FALSE
	switch VAR_RESULT
	case FRONTIER_LVL_TENT, PokemonCenter_B1F_EventScript_SimCancel
	case MULTI_B_PRESSED, PokemonCenter_B1F_EventScript_SimCancel
	frontier_checkineligible
	goto_if_eq VAR_0x8004, TRUE, PokemonCenter_B1F_EventScript_SimNotEnoughMons
	frontier_set FRONTIER_DATA_LVL_MODE, VAR_RESULT
	call_if_eq VAR_FRONTIER_BATTLE_MODE, FRONTIER_MODE_SINGLES, PokemonCenter_B1F_EventScript_SimMsgSelectThree
	call_if_eq VAR_FRONTIER_BATTLE_MODE, FRONTIER_MODE_DOUBLES, PokemonCenter_B1F_EventScript_SimMsgSelectFour
	fadescreen FADE_TO_BLACK
	frontier_get FRONTIER_DATA_LVL_MODE
	copyvar VAR_0x8004, VAR_RESULT
	call PokemonCenter_B1F_EventScript_SimGetPartySize
	special ChoosePartyForBattleFrontier
	goto_if_eq VAR_RESULT, 0, PokemonCenter_B1F_EventScript_SimLoadPartyCancel
	frontier_set FRONTIER_DATA_SELECTED_MON_ORDER
	@ tower_init troca o warp dinâmico; guardamos o caminho de volta ao 1F
	special FolSimulator_SaveDynamicWarp
	tower_init
	special FolSimulator_RestoreDynamicWarp
	tower_set TOWER_DATA_WIN_STREAK_ACTIVE, TRUE
	frontier_set FRONTIER_DATA_PAUSED, FALSE
	special LoadPlayerParty
	special SavePlayerParty
	setvar VAR_0x8004, FRONTIER_UTIL_FUNC_SET_PARTY_ORDER
	call PokemonCenter_B1F_EventScript_SimGetPartySize
	special CallFrontierUtilFunc
	msgbox PokemonCenter_B1F_Text_SimStarting, MSGBOX_DEFAULT
PokemonCenter_B1F_EventScript_SimNextBattle::
	frontier_get FRONTIER_DATA_BATTLE_NUM
	copyvar VAR_TEMP_3, VAR_RESULT
	addvar VAR_TEMP_3, 1
	buffernumberstring STR_VAR_1, VAR_TEMP_3
	msgbox PokemonCenter_B1F_Text_SimOpponentN, MSGBOX_DEFAULT
	tower_setopponent
	tower_getopponentintro 0
	msgbox gStringVar4, MSGBOX_DEFAULT
	waitmessage
	call PokemonCenter_B1F_EventScript_SimDoBattle
	goto_if_eq VAR_RESULT, B_OUTCOME_WON, PokemonCenter_B1F_EventScript_SimWonBattle
	goto PokemonCenter_B1F_EventScript_SimLostSet
	end

PokemonCenter_B1F_EventScript_SimDoBattle::
	closemessage
	frontier_set FRONTIER_DATA_RECORD_DISABLED, TRUE
	special HealPlayerParty
	setvar VAR_0x8004, SPECIAL_BATTLE_TOWER
	setvar VAR_0x8005, 0
	special DoSpecialTrainerBattle
	frontier_restorehelditems
	special HealPlayerParty
	frontier_resetsketch
	frontier_get FRONTIER_DATA_BATTLE_OUTCOME
	return

PokemonCenter_B1F_EventScript_SimWonBattle::
	frontier_incrementstreak
	tower_setbattlewon
	goto_if_eq VAR_RESULT, 7, PokemonCenter_B1F_EventScript_SimWonSet
PokemonCenter_B1F_EventScript_SimAskContinue::
	message PokemonCenter_B1F_Text_SimRestored
	waitmessage
	multichoice 20, 6, MULTI_GO_ON_RETIRE, TRUE
	switch VAR_RESULT
	case 0, PokemonCenter_B1F_EventScript_SimNextBattle
	case 1, PokemonCenter_B1F_EventScript_SimAskRetire
	goto PokemonCenter_B1F_EventScript_SimAskContinue
	end

PokemonCenter_B1F_EventScript_SimAskRetire::
	message BattleFrontier_BattleTowerBattleRoom_Text_CancelYourChallenge
	waitmessage
	multichoicedefault 20, 8, MULTI_YESNO, 1, FALSE
	switch VAR_RESULT
	case 0, PokemonCenter_B1F_EventScript_SimLostSet
	goto PokemonCenter_B1F_EventScript_SimAskContinue
	end

PokemonCenter_B1F_EventScript_SimWonSet::
	tower_set TOWER_DATA_LVL_MODE
	message PokemonCenter_B1F_Text_SimWonSet
	waitmessage
	frontier_givepoints
	msgbox BattleFrontier_Text_ObtainedXBattlePoints, MSGBOX_GETPOINTS
	goto PokemonCenter_B1F_EventScript_SimEnd
	end

PokemonCenter_B1F_EventScript_SimLostSet::
	tower_set TOWER_DATA_LVL_MODE
	tower_set TOWER_DATA_WIN_STREAK_ACTIVE, FALSE
	msgbox PokemonCenter_B1F_Text_SimLost, MSGBOX_DEFAULT
PokemonCenter_B1F_EventScript_SimEnd::
	frontier_set FRONTIER_DATA_CHALLENGE_STATUS, 0
	special LoadPlayerParty
	special HealPlayerParty
	msgbox PokemonCenter_B1F_Text_SimComeAgain, MSGBOX_DEFAULT
	release
	end

PokemonCenter_B1F_EventScript_SimGetPartySize::
	setvar VAR_0x8005, FRONTIER_PARTY_SIZE
	goto_if_eq VAR_FRONTIER_BATTLE_MODE, FRONTIER_MODE_SINGLES, PokemonCenter_B1F_EventScript_SimGetPartySizeEnd
	setvar VAR_0x8005, FRONTIER_DOUBLES_PARTY_SIZE
PokemonCenter_B1F_EventScript_SimGetPartySizeEnd::
	return

PokemonCenter_B1F_EventScript_SimMsgSelectThree::
	msgbox BattleFrontier_BattleTowerLobby_Text_SelectThreeMons, MSGBOX_DEFAULT
	return

PokemonCenter_B1F_EventScript_SimMsgSelectFour::
	msgbox BattleFrontier_BattleTowerLobby_Text_PleaseSelectFourMons, MSGBOX_DEFAULT
	return

PokemonCenter_B1F_EventScript_SimNotEnoughMons::
	msgbox PokemonCenter_B1F_Text_SimNotEnoughMons, MSGBOX_DEFAULT
	goto PokemonCenter_B1F_EventScript_SimCancel
	end

PokemonCenter_B1F_EventScript_SimLoadPartyCancel::
	special LoadPlayerParty
PokemonCenter_B1F_EventScript_SimCancel::
	msgbox PokemonCenter_B1F_Text_SimComeAgain, MSGBOX_DEFAULT
	release
	end

@ -----------------------------------------------------------------------------
@ Lojas de BP (reaproveitam os scripts da Exchange Service Corner e do Lounge 7)
@ -----------------------------------------------------------------------------
PokemonCenter_B1F_EventScript_DecorClerk::
	lock
	faceplayer
	message PokemonCenter_B1F_Text_DecorClerkWhichList
	waitmessage
	multichoice 17, 6, MULTI_FOL_DECOR_SHOP, FALSE
	switch VAR_RESULT
	case 0, PokemonCenter_B1F_EventScript_DecorList1
	case 1, PokemonCenter_B1F_EventScript_DecorList2
	msgbox BattleFrontier_ExchangeServiceCorner_Text_ThankYouVisitWithPoints, MSGBOX_DEFAULT
	release
	end

PokemonCenter_B1F_EventScript_DecorList1::
	setvar VAR_TEMP_2, EXCHANGE_CORNER_DECOR1_CLERK
	call BattleFrontier_ExchangeServiceCorner_EventScript_ClerkWelcome
	goto BattleFrontier_ExchangeServiceCorner_EventScript_ChooseDecor1
	end

PokemonCenter_B1F_EventScript_DecorList2::
	setvar VAR_TEMP_2, EXCHANGE_CORNER_DECOR2_CLERK
	call BattleFrontier_ExchangeServiceCorner_EventScript_ClerkWelcome
	goto BattleFrontier_ExchangeServiceCorner_EventScript_ChooseDecor2
	end

PokemonCenter_B1F_EventScript_ItemClerk::
	lock
	faceplayer
	message PokemonCenter_B1F_Text_ItemClerkWhichList
	waitmessage
	multichoice 17, 6, MULTI_FOL_ITEM_SHOP, FALSE
	switch VAR_RESULT
	case 0, PokemonCenter_B1F_EventScript_ItemList1
	case 1, PokemonCenter_B1F_EventScript_ItemList2
	msgbox BattleFrontier_ExchangeServiceCorner_Text_ThankYouVisitWithPoints, MSGBOX_DEFAULT
	release
	end

PokemonCenter_B1F_EventScript_ItemList1::
	setvar VAR_TEMP_2, EXCHANGE_CORNER_VITAMIN_CLERK
	call BattleFrontier_ExchangeServiceCorner_EventScript_ClerkWelcome
	goto BattleFrontier_ExchangeServiceCorner_EventScript_ChooseVitamin
	end

PokemonCenter_B1F_EventScript_ItemList2::
	setvar VAR_TEMP_2, EXCHANGE_CORNER_HOLD_ITEM_CLERK
	call BattleFrontier_ExchangeServiceCorner_EventScript_ClerkWelcome
	goto BattleFrontier_ExchangeServiceCorner_EventScript_ChooseHoldItem
	end

PokemonCenter_B1F_EventScript_LeftMoveTutor::
	goto BattleFrontier_Lounge7_EventScript_LeftMoveTutor
	end

PokemonCenter_B1F_EventScript_RightMoveTutor::
	goto BattleFrontier_Lounge7_EventScript_RightMoveTutor
	end

@ -----------------------------------------------------------------------------
@ Textos
@ -----------------------------------------------------------------------------
PokemonCenter_B1F_Text_SimWelcome:
	.string "Welcome to the BATTLE SIMULATOR!\p"
	.string "Win seven simulated battles in a\n"
	.string "row to earn Battle Points.$"

PokemonCenter_B1F_Text_SimWhichMode:
	.string "Which kind of battle would you\n"
	.string "like to simulate?$"

PokemonCenter_B1F_Text_SimInfo:
	.string "SINGLE BATTLES use three POKéMON,\n"
	.string "DOUBLE BATTLES use four.\p"
	.string "The rules are the same as in a\n"
	.string "BATTLE TOWER challenge.\p"
	.string "Your POKéMON are healed after\n"
	.string "every battle.\p"
	.string "Win seven in a row and you will\n"
	.string "receive Battle Points.$"

PokemonCenter_B1F_Text_SimNotEnoughMons:
	.string "You don't have enough eligible\n"
	.string "POKéMON for this simulation.\p"
	.string "Each POKéMON must be different\n"
	.string "and hold a different item.\p"
	.string "Some POKéMON may not enter.$"

PokemonCenter_B1F_Text_SimStarting:
	.string "Starting the simulation!$"

PokemonCenter_B1F_Text_SimOpponentN:
	.string "SIMULATION: opponent no. {STR_VAR_1}.$"

PokemonCenter_B1F_Text_SimRestored:
	.string "Your POKéMON were restored.\n"
	.string "Shall we continue?$"

PokemonCenter_B1F_Text_SimWonSet:
	.string "Congratulations! You won all seven\n"
	.string "simulated battles!$"

PokemonCenter_B1F_Text_SimLost:
	.string "The simulation is over.$"

PokemonCenter_B1F_Text_SimComeAgain:
	.string "We hope to see you again!$"

PokemonCenter_B1F_Text_DecorClerkWhichList:
	.string "We exchange Battle Points for\n"
	.string "decorations. Which list?$"

PokemonCenter_B1F_Text_ItemClerkWhichList:
	.string "We exchange Battle Points for\n"
	.string "items. Which list?$"
