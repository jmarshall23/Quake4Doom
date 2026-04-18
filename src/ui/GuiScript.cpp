/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms.
===========================================================================
*/

#include "precompiled.h"
#pragma hdrstop

#include "Window.h"
#include "Winvar.h"
#include "GuiScript.h"
#include "UserInterfaceLocal.h"

/*
===============================================================================

	Script handlers

===============================================================================
*/

idCVar gui_debugScript("gui_debugScript", "0", CVAR_BOOL, "");

/*
=========================
Script_Set
=========================
*/
void Script_Set(idWindow* window, idList<idGSWinVar>* src) {
	idStr key, val;

	if (src->Num() < 2) {
		const char* name = (src->Num() > 0 && (*src)[0].var != NULL) ? (*src)[0].var->c_str() : "unknown";
		common->Warning("Script_Set: src <%s> on window <%s> does not contain value to set to", name, window->GetName());
		return;
	}

	idWinStr* dest = dynamic_cast<idWinStr*>((*src)[0].var);
	if (dest) {
		if (idStr::Icmp(*dest, "cmd") == 0) {
			dest = dynamic_cast<idWinStr*>((*src)[1].var);
			if (!dest) {
				return;
			}

			const int parmCount = src->Num();
			if (parmCount > 2) {
				val = dest->c_str();
				for (int i = 2; i < parmCount; i++) {
					val += " \"";
					val += (*src)[i].var->c_str();
					val += "\"";
				}
				window->AddCommand(val);
			}
			else {
				window->AddCommand(*dest);
			}
			return;
		}
	}

	(*src)[0].var->Set((*src)[1].var->c_str());
	(*src)[0].var->SetEval(false);
}

/*
=========================
Script_SetFocus
=========================
*/
void Script_SetFocus(idWindow* window, idList<idGSWinVar>* src) {
	idWinStr* parm = dynamic_cast<idWinStr*>((*src)[0].var);
	if (parm) {
		drawWin_t* win = window->GetGui()->GetDesktop()->FindChildByName(*parm);
		if (win && win->win) {
			window->SetFocus(win->win);

			// Quake 4 addition: optional second parm "st" clears transitions
			if (src->Num() > 1) {
				idWinStr* opt = dynamic_cast<idWinStr*>((*src)[1].var);
				if (opt && idStr::Icmp(*opt, "st") == 0) {
					win->win->ClearTransitions();
				}
			}
		}
	}
}

/*
=========================
Script_ShowCursor
=========================
*/
void Script_ShowCursor(idWindow* window, idList<idGSWinVar>* src) {
	idWinStr* parm = dynamic_cast<idWinStr*>((*src)[0].var);
	if (parm) {
		if (atoi(*parm)) {
			window->GetGui()->GetDesktop()->ClearFlag(WIN_NOCURSOR);
		}
		else {
			window->GetGui()->GetDesktop()->SetFlag(WIN_NOCURSOR);
		}
	}
}

/*
=========================
Script_RunScript

 run scripts must come after any set cmd set's in the script
=========================
*/
void Script_RunScript(idWindow* window, idList<idGSWinVar>* src) {
	idWinStr* parm = dynamic_cast<idWinStr*>((*src)[0].var);
	if (parm) {
		idStr str = window->cmd;
		str += " ; runScript ";
		str += parm->c_str();
		window->cmd = str;
	}
}

/*
=========================
Script_LocalSound
=========================
*/
void Script_LocalSound(idWindow* window, idList<idGSWinVar>* src) {
	idWinStr* parm = dynamic_cast<idWinStr*>((*src)[0].var);
	if (parm) {
		session->sw->PlayShaderDirectly(*parm);
	}
}

/*
=========================
Script_EvalRegs
=========================
*/
void Script_EvalRegs(idWindow* window, idList<idGSWinVar>* src) {
	window->EvalRegs(-1, true);
}

/*
=========================
Script_EndGame
=========================
*/
void Script_EndGame(idWindow* window, idList<idGSWinVar>* src) {
	cvarSystem->SetCVarBool("g_nightmare", true);
	cmdSystem->BufferCommandText(CMD_EXEC_APPEND, "disconnect\n");
}

/*
=========================
Script_ResetTime
=========================
*/
void Script_ResetTime(idWindow* window, idList<idGSWinVar>* src) {
	idWinStr* parm = dynamic_cast<idWinStr*>((*src)[0].var);
	drawWin_t* win = NULL;

	if (parm && src->Num() > 1) {
		win = window->GetGui()->GetDesktop()->FindChildByName(*parm);
		parm = dynamic_cast<idWinStr*>((*src)[1].var);
	}

	if (!parm) {
		return;
	}

	if (win && win->win) {
		win->win->ResetTime(atoi(*parm));
		win->win->EvalRegs(-1, true);
	}
	else {
		window->ResetTime(atoi(*parm));
		window->EvalRegs(-1, true);
	}
}

/*
=========================
Script_ResetCinematics
=========================
*/
void Script_ResetCinematics(idWindow* window, idList<idGSWinVar>* src) {
	window->ResetCinematics();
}

/*
=========================
Script_ResetVideo
=========================
*/
void Script_ResetVideo(idWindow* window, idList<idGSWinVar>* src) {
	idWinStr* parm = dynamic_cast<idWinStr*>((*src)[0].var);
	if (!parm) {
		window->ResetCinematics();
		window->EvalRegs(-1, true);
		return;
	}

	drawWin_t* child = window->GetGui()->GetDesktop()->FindChildByName(*parm);
	if (child) {
		if (child->win) {
			child->win->ResetCinematics();
			child->win->EvalRegs(-1, true);
			return;
		}
		if (child->simp) {
			child->simp->ResetCinematics();
			return;
		}
	}

	window->ResetCinematics();
	window->EvalRegs(-1, true);
}

/*
=========================
Script_SetLightColor
=========================
*/
void Script_SetLightColor(idWindow* window, idList<idGSWinVar>* src) {

}

/*
=========================
Script_NonInteractive
=========================
*/
void Script_NonInteractive(idWindow* window, idList<idGSWinVar>* src) {
	idWinStr* parm = NULL;
	if (src->Num() > 0) {
		parm = dynamic_cast<idWinStr*>((*src)[0].var);
	}

	if (parm) {
		const bool nonInteractive = (atoi(parm->c_str()) != 0);
		window->GetGui()->SetInteractive(!nonInteractive);
	}
	else {
		window->GetGui()->SetInteractive(false);
	}
}

/*
=========================
Script_NamedEvent
=========================
*/
void Script_NamedEvent(idWindow* window, idList<idGSWinVar>* src) {
	idWinStr* parm = dynamic_cast<idWinStr*>((*src)[0].var);
	if (!parm) {
		return;
	}

	idStr event = parm->c_str();
	int split = idStr::FindText(event.c_str(), "::", true, 0, event.Length());
	if (split <= 0) {
		window->GetGui()->HandleNamedEvent(event);
		return;
	}

	idStr winName;
	idStr winEvent;
	event.Left(split, winName);
	event.Right(event.Length() - split - 2, winEvent);

	drawWin_t* child = window->GetGui()->GetDesktop()->FindChildByName(winName);
	if (child && child->win) {
		child->win->RunNamedEvent(winEvent);
	}
	else if (gui_debugScript.GetBool()) {
		common->Printf("GUI: %s: unknown window %s for named event %s\n",
			window->GetName(), winName.c_str(), winEvent.c_str());
	}
}

/*
=========================
Script_StopTransitions
=========================
*/
void Script_StopTransitions(idWindow* window, idList<idGSWinVar>* src) {
	idWinStr* parm = dynamic_cast<idWinStr*>((*src)[0].var);
	if (!parm) {
		return;
	}

	drawWin_t* child = window->GetGui()->GetDesktop()->FindChildByName(*parm);
	if (child && child->win) {
		child->win->ClearTransitions();
	}
	else if (gui_debugScript.GetBool()) {
		common->Printf("GUI: %s %s: unknown window for Script_StopTransitions\n",
			window->GetName(), parm->c_str());
	}
}

/*
=========================
Script_ConsoleCmd
=========================
*/
void Script_ConsoleCmd(idWindow* window, idList<idGSWinVar>* src) {
	idWinStr* parm = dynamic_cast<idWinStr*>((*src)[0].var);
	if (!parm) {
		return;
	}

	idStr val = parm->c_str();
	val += " ";
	val += "\n";
	cmdSystem->BufferCommandText(CMD_EXEC_APPEND, val);
}

/*
=========================
Script_Transition
=========================
*/
void Script_Transition(idWindow* window, idList<idGSWinVar>* src) {
	// transitions always affect rect, vec4, float, or float member vars
	if (src->Num() >= 4) {
		idWinRectangle* rect = NULL;
		idWinVec4* vec4 = dynamic_cast<idWinVec4*>((*src)[0].var);
		idWinFloat* val = NULL;
		idWinFloatMember* floatMember = NULL;

		if (vec4 == NULL) {
			rect = dynamic_cast<idWinRectangle*>((*src)[0].var);
			if (rect == NULL) {
				val = dynamic_cast<idWinFloat*>((*src)[0].var);
				if (val == NULL) {
					floatMember = dynamic_cast<idWinFloatMember*>((*src)[0].var);
				}
			}
		}

		idVec4 from;
		idVec4 to;

		idWinVec4* fromVec4 = dynamic_cast<idWinVec4*>((*src)[1].var);
		if (fromVec4) {
			from = *fromVec4;
		}
		else {
			float f;
			idWinFloat* fromFloat = dynamic_cast<idWinFloat*>((*src)[1].var);
			idWinFloatMember* fromFloatMember = dynamic_cast<idWinFloatMember*>((*src)[1].var);
			if (fromFloat) {
				f = (float)atof(fromFloat->c_str());
			}
			else if (fromFloatMember) {
				f = (float)atof(fromFloatMember->c_str());
			}
			else {
				f = (float)atof((*src)[1].var->c_str());
			}
			from = idVec4(f, f, f, f);
		}

		idWinVec4* toVec4 = dynamic_cast<idWinVec4*>((*src)[2].var);
		if (toVec4) {
			to = *toVec4;
		}
		else {
			float f;
			idWinFloat* toFloat = dynamic_cast<idWinFloat*>((*src)[2].var);
			idWinFloatMember* toFloatMember = dynamic_cast<idWinFloatMember*>((*src)[2].var);
			if (toFloat) {
				f = (float)atof(toFloat->c_str());
			}
			else if (toFloatMember) {
				f = (float)atof(toFloatMember->c_str());
			}
			else {
				f = (float)atof((*src)[2].var->c_str());
			}
			to = idVec4(f, f, f, f);
		}

		int time;
		idWinInt* timeInt = dynamic_cast<idWinInt*>((*src)[3].var);
		if (timeInt) {
			time = atoi(timeInt->c_str());
		}
		else {
			time = atoi((*src)[3].var->c_str());
		}

		float ac = 0.0f;
		float dc = 0.0f;

		if (src->Num() > 4) {
			ac = (float)atof((*src)[4].var->c_str());
			if (src->Num() > 5) {
				dc = (float)atof((*src)[5].var->c_str());
			}
		}

		if (!((vec4 || rect || val || floatMember))) {
			common->Warning("Bad transition in gui %s in window %s\n",
				window->GetGui()->GetSourceFile(), window->GetName());
			return;
		}

		if (vec4) {
			vec4->SetEval(false);
			window->AddTransition(vec4, from, to, time, ac, dc);
		}
		else if (val) {
			val->SetEval(false);
			window->AddTransition(val, from, to, time, ac, dc);
		}
		else if (floatMember) {
			floatMember->SetEval(false);
			window->AddTransition(floatMember, from, to, time, ac, dc);
		}
		else {
			rect->SetEval(false);
			window->AddTransition(rect, from, to, time, ac, dc);
		}

		window->StartTransition();
	}
}

typedef struct {
	const char* name;
	void (*handler)(idWindow* window, idList<idGSWinVar>* src);
	int mMinParms;
	int mMaxParms;
} guiCommandDef_t;

guiCommandDef_t commandList[] = {
	{ "set",				Script_Set,				2, 999 },
	{ "setFocus",			Script_SetFocus,		1, 2 },
	{ "endGame",			Script_EndGame,			0, 0 },
	{ "resetTime",			Script_ResetTime,		0, 2 },
	{ "showCursor",			Script_ShowCursor,		1, 1 },
	{ "resetCinematics",	Script_ResetCinematics,	0, 2 },
	{ "transition",			Script_Transition,		4, 6 },
	{ "localSound",			Script_LocalSound,		1, 1 },
	{ "runScript",			Script_RunScript,		1, 1 },
	{ "evalRegs",			Script_EvalRegs,		0, 0 },

	// Quake 4 additions
	{ "setLightColor",		Script_SetLightColor,	1, 1 },
	{ "nonInteractive",		Script_NonInteractive,	0, 1 },
	{ "resetVideo",			Script_ResetVideo,		1, 1 },
	{ "namedEvent",			Script_NamedEvent,		1, 1 },
	{ "stopTransitions",	Script_StopTransitions,	1, 1 },
	{ "consoleCmd",			Script_ConsoleCmd,		1, 1 }
};

int scriptCommandCount = sizeof(commandList) / sizeof(guiCommandDef_t);

/*
=========================
idGuiScript::idGuiScript
=========================
*/
idGuiScript::idGuiScript() {
	ifList = NULL;
	elseList = NULL;
	conditionReg = -1;
	handler = NULL;
	parms.SetGranularity(2);
}

/*
=========================
idGuiScript::~idGuiScript
=========================
*/
idGuiScript::~idGuiScript() {
	delete ifList;
	delete elseList;
	const int c = parms.Num();
	for (int i = 0; i < c; i++) {
		if (parms[i].own) {
			delete parms[i].var;
		}
	}
}

/*
=========================
idGuiScript::WriteToSaveGame
=========================
*/
void idGuiScript::WriteToSaveGame(idFile* savefile) {
	if (ifList) {
		ifList->WriteToSaveGame(savefile);
	}
	if (elseList) {
		elseList->WriteToSaveGame(savefile);
	}

	savefile->Write(&conditionReg, sizeof(conditionReg));

	for (int i = 0; i < parms.Num(); i++) {
		if (parms[i].own) {
			parms[i].var->WriteToSaveGame(savefile);
		}
	}
}

/*
=========================
idGuiScript::ReadFromSaveGame
=========================
*/
void idGuiScript::ReadFromSaveGame(idFile* savefile) {
	if (ifList) {
		ifList->ReadFromSaveGame(savefile);
	}
	if (elseList) {
		elseList->ReadFromSaveGame(savefile);
	}

	savefile->Read(&conditionReg, sizeof(conditionReg));

	for (int i = 0; i < parms.Num(); i++) {
		if (parms[i].own) {
			parms[i].var->ReadFromSaveGame(savefile);
		}
	}
}

/*
=========================
idGuiScript::Parse
=========================
*/
bool idGuiScript::Parse(idParser* src) {
	int i;

	idToken token;
	if (!src->ReadToken(&token)) {
		src->Error("Unexpected end of file");
		return false;
	}

	handler = NULL;

	for (i = 0; i < scriptCommandCount; i++) {
		if (idStr::Icmp(token, commandList[i].name) == 0) {
			handler = commandList[i].handler;
			break;
		}
	}

	if (handler == NULL) {
		src->Error("Uknown script call %s", token.c_str());
	}

	while (1) {
		if (!src->ReadToken(&token)) {
			src->Error("Unexpected end of file");
			return false;
		}

		if (idStr::Icmp(token, ";") == 0) {
			break;
		}

		if (idStr::Icmp(token, "}") == 0) {
			src->UnreadToken(&token);
			break;
		}

		idWinStr* str = new idWinStr();
		*str = token;

		idGSWinVar wv;
		wv.own = true;
		wv.var = str;
		parms.Append(wv);
	}

	if (handler && (parms.Num() < commandList[i].mMinParms || parms.Num() > commandList[i].mMaxParms)) {
		src->Error("incorrect number of parameters for script %s", commandList[i].name);
	}

	return true;
}

/*
=========================
idGuiScriptList::Execute
=========================
*/
void idGuiScriptList::Execute(idWindow* win) {
	const int c = list.Num();

	if (gui_debugScript.GetBool()) {
		common->Printf("GUI: %s: commands:\n", win->GetName());
	}

	for (int i = 0; i < c; i++) {
		idGuiScript* gs = list[i];
		assert(gs);

		if (gs->conditionReg >= 0) {
			if (win->HasOps()) {
				float f = win->EvalRegs(gs->conditionReg);
				if (f) {
					if (gs->ifList) {
						win->RunScriptList(gs->ifList);
					}
				}
				else if (gs->elseList) {
					win->RunScriptList(gs->elseList);
				}
			}
		}

		gs->Execute(win);
	}
}

/*
=========================
idGuiScript::FixupParms
=========================
*/
void idGuiScript::FixupParms(idWindow* win) {
	if (handler == &Script_Set) {
		bool precacheBackground = false;
		bool precacheSounds = false;

		idWinStr* str = dynamic_cast<idWinStr*>(parms[0].var);
		assert(str);

		idWinVar* dest = win->GetWinVarByName(*str, true);
		if (dest) {
			delete parms[0].var;
			parms[0].var = dest;
			parms[0].own = false;

			if (dynamic_cast<idWinBackground*>(dest) != NULL) {
				precacheBackground = true;
			}
		}
		else if (idStr::Icmp(str->c_str(), "cmd") == 0) {
			precacheSounds = true;
		}

		const int parmCount = parms.Num();
		for (int i = 1; i < parmCount; i++) {
			idWinStr* str = dynamic_cast<idWinStr*>(parms[i].var);
			if (!str) {
				continue;
			}

			if (idStr::Icmpn(str->c_str(), "gui::", 5) == 0 || idStr::Icmpn(str->c_str(), "$gui::", 6) == 0) {
				const char* varName = str->c_str();
				if (varName[0] == '$') {
					varName++;
				}

				idWinStr* defvar = new idWinStr();
				defvar->Init(varName, win);
				win->AddDefinedVar(defvar);

				delete parms[i].var;
				parms[i].var = defvar;
				parms[i].own = false;
			}
			else if ((*str)[0] == '$') {
				dest = win->GetGui()->GetDesktop()->GetWinVarByName((const char*)(*str) + 1, true);
				if (dest) {
					delete parms[i].var;
					parms[i].var = dest;
					parms[i].own = false;
				}
			}
			else if (idStr::Cmpn(str->c_str(), STRTABLE_ID, STRTABLE_ID_LENGTH) == 0) {
				str->Set(common->GetLanguageDict()->GetString(str->c_str()));
			}
			else if (precacheBackground) {
				const idMaterial* mat = declManager->FindMaterial(str->c_str());
				mat->SetSort(SS_GUI);
			}
			else if (precacheSounds) {
				idToken token;
				idParser parser(LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT);
				parser.LoadMemory(str->c_str(), str->Length(), "command");

				while (parser.ReadToken(&token)) {
					if (token.Icmp("play") == 0 || token.Icmp("music") == 0) {
						if (parser.ReadToken(&token) && (token != "")) {
							declManager->FindSound(token.c_str());
						}
					}
				}
			}
		}
	}
	else if (handler == &Script_Transition) {
		if (parms.Num() < 4) {
			common->Warning("Window %s in gui %s has a bad transition definition",
				win->GetName(), win->GetGui()->GetSourceFile());
			handler = NULL;
			return;
		}

		idWinStr* str = dynamic_cast<idWinStr*>(parms[0].var);
		assert(str);

		drawWin_t* destowner = NULL;
		idWinVar* dest = win->GetWinVarByName(*str, true, &destowner);

		const bool validDest =
			(dynamic_cast<idWinVec4*>(dest) != NULL) ||
			(dynamic_cast<idWinRectangle*>(dest) != NULL) ||
			(dynamic_cast<idWinFloat*>(dest) != NULL) ||
			(dynamic_cast<idWinFloatMember*>(dest) != NULL);

		if (!validDest) {
			common->Warning("Window %s in gui %s: a transition does not have a valid destination var %s",
				win->GetName(), win->GetGui()->GetSourceFile(), str->c_str());
			handler = NULL;
			return;
		}

		delete parms[0].var;
		parms[0].var = dest;
		parms[0].own = false;

		for (int c = 1; c < 3; c++) {
			idWinStr* str = dynamic_cast<idWinStr*>(parms[c].var);
			assert(str);

			if ((*str)[0] == '$') {
				drawWin_t* owner = NULL;
				idWinVar* parmVar = win->GetWinVarByName((const char*)(*str) + 1, true, &owner);

				const bool ok =
					(dynamic_cast<idWinVec4*>(dest) != NULL && dynamic_cast<idWinVec4*>(parmVar) != NULL) ||
					(dynamic_cast<idWinFloat*>(dest) != NULL &&
						(dynamic_cast<idWinFloat*>(parmVar) != NULL || dynamic_cast<idWinFloatMember*>(parmVar) != NULL)) ||
					(dynamic_cast<idWinFloatMember*>(dest) != NULL &&
						(dynamic_cast<idWinFloat*>(parmVar) != NULL || dynamic_cast<idWinFloatMember*>(parmVar) != NULL)) ||
					(dynamic_cast<idWinRectangle*>(dest) != NULL &&
						(dynamic_cast<idWinVec4*>(parmVar) != NULL || dynamic_cast<idWinRectangle*>(parmVar) != NULL));

				if (!ok) {
					common->Warning("Window %s in gui %s: transition has an invalid parameter %d (%s)",
						win->GetName(), win->GetGui()->GetSourceFile(), c, str->c_str());
					handler = NULL;
					return;
				}

				if (dynamic_cast<idWinRectangle*>(dest) != NULL &&
					dynamic_cast<idWinRectangle*>(parmVar) != NULL) {
					idWinVec4* v4 = new idWinVec4;
					parms[c].var = v4;
					parms[c].own = true;

					idWindow* ownerparent = owner ? (owner->simp ? owner->simp->GetParent() : owner->win->GetParent()) : NULL;
					idWindow* destparent = destowner ? (destowner->simp ? destowner->simp->GetParent() : destowner->win->GetParent()) : NULL;

					if (ownerparent && destparent) {
						idRectangle rect = *(dynamic_cast<idWinRectangle*>(parmVar));
						ownerparent->ClientToScreen(&rect);
						destparent->ScreenToClient(&rect);
						*v4 = rect.ToVec4();
					}
					else {
						v4->Set(parmVar->c_str());
					}

					delete str;
				}
				else {
					delete parms[c].var;
					parms[c].var = parmVar;
					parms[c].own = false;
				}
			}
			else {
				idStr strValue = str->c_str();
				idWinVar* newVar = NULL;

				if (dynamic_cast<idWinVec4*>(dest) != NULL || dynamic_cast<idWinRectangle*>(dest) != NULL) {
					newVar = new idWinVec4;
				}
				else {
					newVar = new idWinFloat;
				}

				delete parms[c].var;
				parms[c].var = newVar;
				parms[c].own = true;
				newVar->Set(strValue.c_str());
			}
		}

		{
			idWinStr* timeStr = dynamic_cast<idWinStr*>(parms[3].var);
			if (timeStr) {
				idWinInt* timeVar = new idWinInt;
				timeVar->Set(timeStr->c_str());
				delete parms[3].var;
				parms[3].var = timeVar;
				parms[3].own = true;
			}
		}

		for (int i = 4; i < parms.Num() && i < 6; i++) {
			idWinStr* fltStr = dynamic_cast<idWinStr*>(parms[i].var);
			if (fltStr) {
				idWinFloat* flt = new idWinFloat;
				flt->Set(fltStr->c_str());
				delete parms[i].var;
				parms[i].var = flt;
				parms[i].own = true;
			}
		}
	}
	else if (handler == &Script_SetLightColor) {
		idWinStr* str = dynamic_cast<idWinStr*>(parms[0].var);
		if (!str) {
			handler = NULL;
			return;
		}

		idWinVar* dest = win->GetWinVarByName(*str, true);
		if (dest) {
			delete parms[0].var;
			parms[0].var = dest;
			parms[0].own = false;
		}
		else {
			common->Warning("Window %s in gui %s: setLightColor command does not specify a valid var %s",
				win->GetName(), win->GetGui()->GetSourceFile(), str->c_str());
			handler = NULL;
		}
	}
	else {
		const int c = parms.Num();
		for (int i = 0; i < c; i++) {
			parms[i].var->Init(parms[i].var->c_str(), win);
		}
	}
}

/*
=========================
idGuiScriptList::FixupParms
=========================
*/
void idGuiScriptList::FixupParms(idWindow* win) {
	const int c = list.Num();
	for (int i = 0; i < c; i++) {
		idGuiScript* gs = list[i];
		gs->FixupParms(win);
		if (gs->ifList) {
			gs->ifList->FixupParms(win);
		}
		if (gs->elseList) {
			gs->elseList->FixupParms(win);
		}
	}
}

/*
=========================
idGuiScriptList::WriteToSaveGame
=========================
*/
void idGuiScriptList::WriteToSaveGame(idFile* savefile) {
	for (int i = 0; i < list.Num(); i++) {
		list[i]->WriteToSaveGame(savefile);
	}
}

/*
=========================
idGuiScriptList::ReadFromSaveGame
=========================
*/
void idGuiScriptList::ReadFromSaveGame(idFile* savefile) {
	for (int i = 0; i < list.Num(); i++) {
		list[i]->ReadFromSaveGame(savefile);
	}
}