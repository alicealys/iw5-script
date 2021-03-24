#pragma once

#define WEAK __declspec(selectany)

namespace game
{
	// Functions

	WEAK symbol<void(int type, VariableUnion u)> AddRefToValue{0x5656E0};
	WEAK symbol<void(int type, VariableUnion u)> RemoveRefToValue{0x565730};

	WEAK symbol<void(int localClientNum, const char* text)> Cbuf_AddText{0x0};
	WEAK symbol<void(const char* cmdName, void(), cmd_function_t* allocedCmd)> Cmd_AddCommandInternal{0x0};
	WEAK symbol<const char*(int index)> Cmd_Argv{0x0};

	WEAK symbol<const dvar_t*(const char*)> Dvar_FindVar{0x0};

	WEAK symbol<char*(const char*)> I_CleanStr{0x0};

	WEAK symbol<VariableValue(unsigned int classnum, int entnum, int offset)> GetEntityFieldValue{0x56AF20};

	WEAK symbol<const float* (const float* v)> Scr_AllocVector{0x565680};
	//WEAK symbol<void()> Scr_ClearOutParams{0x569010};
	WEAK symbol<scr_entref_t(unsigned int entId)> Scr_GetEntityIdRef{0x565F60};
	WEAK symbol<void(unsigned int classnum, int entnum, int offset)> Scr_SetObjectField{0x52BCC0};
	WEAK symbol<void(int id, unsigned int stringValue, unsigned int paramcount)> Scr_NotifyId{0x56B5E0};

	WEAK symbol<unsigned int(const char* str, unsigned int user)> SL_GetString{0x5649E0};
	WEAK symbol<const char*(unsigned int stringValue)> SL_ConvertToString{0x564270};

	WEAK symbol<void(int clientNum, int type, const char* command)> SV_GameSendServerCommand{0x0};

	WEAK symbol<void* (jmp_buf* Buf, int Value)> longjmp{0x7363BC};
	WEAK symbol<int(jmp_buf* Buf)> _setjmp{0x734CF8};

	// Variables

	WEAK symbol<int> g_script_error_level{0x20B21FC};
	WEAK symbol<jmp_buf> g_script_error{0x20B4218};

	WEAK symbol<scrVmPub_t> scr_VmPub{0x20B4A80};
	WEAK symbol<scrVarGlob_t> scr_VarGlob{0x1E72180};

	WEAK symbol<scr_classStruct_t*> g_classMap{0x8B4300};

	WEAK symbol<gentity_s> g_entities{0x0};
	WEAK symbol<unsigned int> levelEntityId{0x208E1A4};
}