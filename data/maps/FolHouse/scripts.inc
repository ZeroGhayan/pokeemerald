@ HACKROM (Forest of Love): casas de viagem rápida.
@ MAP_FOL_HOUSE é o ÚNICO interior de todas as casas (inclusive o quarto de
@ Littleroot depois da intro). Lógica em src/fol_houses.c; guia em
@ docs/casas_fast_travel.md.

FolHouse_MapScripts::
	map_script MAP_SCRIPT_ON_WARP_INTO_MAP_TABLE, FolHouse_MapOnWarp
	map_script MAP_SCRIPT_ON_TRANSITION, FolHouse_MapOnTransition
	map_script MAP_SCRIPT_ON_RESUME, FolHouse_MapOnResume
	.byte 0

@ VAR_TEMP_1 zera a cada warp: as decorações-sprite (bonecos, almofadas) são
@ criadas uma vez por entrada, como o VAR_SECRET_BASE_INITIALIZED vanilla.
FolHouse_MapOnWarp:
	map_script_2 VAR_TEMP_1, 0, FolHouse_EventScript_InitDecorationSprites
	.2byte 0

FolHouse_MapOnTransition:
	call SecretBase_EventScript_SetDecorationFlags
	special FolHouse_OnTransition
	end

FolHouse_MapOnResume:
	setstepcallback STEP_CB_SECRET_BASE
	end

FolHouse_EventScript_InitDecorationSprites::
	setvar VAR_0x8004, 0
	setvar VAR_0x8005, 0
	special InitSecretBaseDecorationSprites
	setvar VAR_TEMP_1, 1
	end

FolHouse_EventScript_Release::
	releaseall
	end

@ -----------------------------------------------------------------------------
@ PC: viagem rápida + decoração (fixa/móvel). Sem saque de itens nas casas.
@ (field_control_avatar.c manda o metatile de PC de Secret Base para cá.)
@ -----------------------------------------------------------------------------
FolHouse_EventScript_PC::
	lockall
	playse SE_PC_LOGIN
	message SecretBase_Text_BootUpPC
	dofieldeffect FLDEFF_PCTURN_ON
	waitstate
	waitmessage
	waitbuttonpress
	playse SE_SELECT
FolHouse_EventScript_PCMainMenu::
	message SecretBase_Text_WhatWouldYouLikeToDo
	waitmessage
	multichoice 0, 0, MULTI_FOL_HOUSE_PC, FALSE
	switch VAR_RESULT
	case 0, FolHouse_EventScript_PCFastTravel
	case 1, FolHouse_EventScript_PCDecorMenu
	goto FolHouse_EventScript_PCTurnOff
	end

FolHouse_EventScript_PCTurnOff::
	closemessage
	special DoSecretBasePCTurnOffEffect
	releaseall
	end

FolHouse_EventScript_PCFastTravel::
	setvar VAR_0x8004, FOL_LIST_FAST_TRAVEL
	message FolHouse_Text_WhereToTravel
	waitmessage
	special FolHouse_ShowList
	goto_if_eq VAR_RESULT, FOL_HOUSE_NONE, FolHouse_EventScript_NoOtherHouse
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, FolHouse_EventScript_PCMainMenu
	copyvar VAR_0x8004, VAR_RESULT
	special FolHouse_BufferHouseInfo
	msgbox FolHouse_Text_TravelingTo, MSGBOX_DEFAULT
	closemessage
	special DoSecretBasePCTurnOffEffect
	special FolHouse_PrepareFastTravel
	playse SE_WARP_IN
	warp MAP_FOL_HOUSE, FOL_HOUSE_PC_FRONT_X, FOL_HOUSE_PC_FRONT_Y
	waitstate
	end

FolHouse_EventScript_NoOtherHouse::
	msgbox FolHouse_Text_NoOtherHouse, MSGBOX_DEFAULT
	goto FolHouse_EventScript_PCMainMenu
	end

@ Chamado também pelo menu vanilla de decoração ao cancelar (decoration.c).
FolHouse_EventScript_PCDecorMenu::
	lockall
	message SecretBase_Text_WhatWouldYouLikeToDo
	waitmessage
	multichoice 0, 0, MULTI_FOL_HOUSE_DECOR, FALSE
	switch VAR_RESULT
	case 0, FolHouse_EventScript_PCFixedDecor
	case 1, FolHouse_EventScript_PCMovableDecor
	goto FolHouse_EventScript_PCMainMenu
	end

FolHouse_EventScript_PCMovableDecor::
	special ShowSecretBaseDecorationMenu
	end

FolHouse_EventScript_PCFixedDecor::
	message FolHouse_Text_WhichFixedDecor
	waitmessage
	setvar VAR_0x8004, FOL_LIST_FIXED_CATEGORY
	special FolHouse_ShowList
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, FolHouse_EventScript_PCDecorMenu
	copyvar VAR_0x8005, VAR_RESULT
	message FolHouse_Text_WhichOption
	waitmessage
	setvar VAR_0x8004, FOL_LIST_FIXED_OPTION
	special FolHouse_ShowList
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, FolHouse_EventScript_PCFixedDecor
	copyvar VAR_0x8004, VAR_0x8005
	copyvar VAR_0x8005, VAR_RESULT
	special FolHouse_SetFixedDecor
	closemessage
	special DoSecretBasePCTurnOffEffect
	@ Re-entra no mapa para redesenhar as decorações fixas.
	warp MAP_FOL_HOUSE, FOL_HOUSE_PC_FRONT_X, FOL_HOUSE_PC_FRONT_Y
	waitstate
	end

@ -----------------------------------------------------------------------------
@ Decorações fixas
@ -----------------------------------------------------------------------------
FolHouse_EventScript_Bed::
	lockall
	msgbox FolHouse_Text_BedRest, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, FolHouse_EventScript_Release
	call Common_EventScript_SaveGame
	goto_if_eq VAR_RESULT, 0, FolHouse_EventScript_Release
	msgbox FolHouse_Text_BedReturnToTitle, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, FolHouse_EventScript_Release
	closemessage
	fadescreen FADE_TO_BLACK
	special FolHouse_ReturnToTitle
	end

FolHouse_EventScript_TV::
	goto EventScript_TV
	end

FolHouse_EventScript_Console::
	msgbox FolHouse_Text_Console, MSGBOX_SIGN
	end

FolHouse_EventScript_Bathtub::
	msgbox FolHouse_Text_Bathtub, MSGBOX_SIGN
	end

FolHouse_EventScript_Stove::
	msgbox FolHouse_Text_Stove, MSGBOX_SIGN
	end

FolHouse_EventScript_Fridge::
	msgbox FolHouse_Text_Fridge, MSGBOX_SIGN
	end

FolHouse_EventScript_PcSide::
	lockall
	special FolHouse_GetPcSideOption
	goto_if_eq VAR_RESULT, FOL_PC_SIDE_TROPHY, FolHouse_EventScript_Trophy
	goto_if_eq VAR_RESULT, FOL_PC_SIDE_PHONE, FolHouse_EventScript_Phone
FolHouse_EventScript_Radio::
	message FolHouse_Text_RadioWhichTrack
	waitmessage
	setvar VAR_0x8004, FOL_LIST_RADIO
	special FolHouse_ShowList
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, FolHouse_EventScript_RadioClose
	copyvar VAR_0x8004, VAR_RESULT
	special FolHouse_SetRadioTrack
FolHouse_EventScript_RadioClose::
	closemessage
	releaseall
	end

FolHouse_EventScript_Trophy::
	msgbox FolHouse_Text_Trophy, MSGBOX_DEFAULT
	releaseall
	end

FolHouse_EventScript_Phone::
	special FolHouse_PhoneHook
	msgbox FolHouse_Text_PhoneNobody, MSGBOX_DEFAULT
	releaseall
	end

@ -----------------------------------------------------------------------------
@ Agentes imobiliários (entradas de TESTE nas cidades). Cada cidade só precisa
@ de um objeto apontando para um wrapper que define VAR_0x8004 = FOL_HOUSE_*.
@ -----------------------------------------------------------------------------
FolHouse_EventScript_AgentOldale::
	setvar VAR_0x8004, FOL_HOUSE_OLDALE
	goto FolHouse_EventScript_Agent
	end

FolHouse_EventScript_AgentPetalburg::
	setvar VAR_0x8004, FOL_HOUSE_PETALBURG
	goto FolHouse_EventScript_Agent
	end

FolHouse_EventScript_AgentRustboro::
	setvar VAR_0x8004, FOL_HOUSE_RUSTBORO
	goto FolHouse_EventScript_Agent
	end

FolHouse_EventScript_Agent::
	lock
	faceplayer
	special FolHouse_BufferHouseInfo
	special FolHouse_CheckOwned
	goto_if_eq VAR_RESULT, TRUE, FolHouse_EventScript_AgentOwned
	showmoneybox 0, 0
	msgbox FolHouse_Text_AgentForSale, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, FolHouse_EventScript_AgentDecline
	special FolHouse_TryBuy
	goto_if_eq VAR_RESULT, FALSE, FolHouse_EventScript_AgentNoMoney
	updatemoneybox
	playse SE_SHOP
	msgbox FolHouse_Text_AgentBought, MSGBOX_DEFAULT
	hidemoneybox
FolHouse_EventScript_AgentOwned::
	message FolHouse_Text_AgentWelcomeHome
	waitmessage
	multichoice 20, 8, MULTI_FOL_HOUSE_AGENT, FALSE
	goto_if_ne VAR_RESULT, 0, FolHouse_EventScript_AgentBye
	closemessage
	special FolHouse_PrepareEnter
	playse SE_EXIT
	warp MAP_FOL_HOUSE, FOL_HOUSE_DOOR_WARP
	waitstate
	end

FolHouse_EventScript_AgentNoMoney::
	msgbox FolHouse_Text_AgentNoMoney, MSGBOX_DEFAULT
FolHouse_EventScript_AgentDecline::
	hidemoneybox
FolHouse_EventScript_AgentBye::
	msgbox FolHouse_Text_AgentBye, MSGBOX_DEFAULT
	release
	end

@ -----------------------------------------------------------------------------
@ Textos (em inglês, como o resto do jogo)
@ -----------------------------------------------------------------------------
FolHouse_Text_WhereToTravel:
	.string "Travel to which house?$"

FolHouse_Text_TravelingTo:
	.string "Fast traveling to your house in\n{STR_VAR_3}!$"

FolHouse_Text_NoOtherHouse:
	.string "You don't own any other house yet.$"

FolHouse_Text_WhichFixedDecor:
	.string "Change which furniture?$"

FolHouse_Text_WhichOption:
	.string "Choose a style.$"

FolHouse_Text_BedRest:
	.string "It's your cozy bed.\n"
	.string "Rest for a while?$"

FolHouse_Text_BedReturnToTitle:
	.string "Return to the title screen?$"

FolHouse_Text_Console:
	.string "It's a game console.\n"
	.string "Better get back to the adventure!$"

FolHouse_Text_Bathtub:
	.string "A shiny bathtub.\n"
	.string "The water looks nice and warm.$"

FolHouse_Text_Stove:
	.string "It's a stove.\n"
	.string "Something smells delicious…$"

FolHouse_Text_Fridge:
	.string "The fridge is full of snacks\n"
	.string "and drinks.$"

FolHouse_Text_RadioWhichTrack:
	.string "It's a radio.\n"
	.string "Which station will you listen to?$"

FolHouse_Text_Trophy:
	.string "The SYLVARIA LEAGUE trophy!\n"
	.string "Proof of your victory as CHAMPION.$"

FolHouse_Text_PhoneNobody:
	.string "It's a phone.\n"
	.string "No one is calling right now.$"

FolHouse_Text_AgentForSale:
	.string "This lovely house in {STR_VAR_3}\n"
	.string "is for sale for ¥{STR_VAR_2}.\p"
	.string "All your houses share the same\n"
	.string "furniture, and their PCs let you\l"
	.string "travel between them.\p"
	.string "Would you like to buy it?$"

FolHouse_Text_AgentBought:
	.string "Thank you! The house is yours.$"

FolHouse_Text_AgentNoMoney:
	.string "Oh… You don't have enough money.$"

FolHouse_Text_AgentWelcomeHome:
	.string "Would you like to go inside?$"

FolHouse_Text_AgentBye:
	.string "Come back anytime!$"
