#pragma once

#define WEAK __declspec(selectany)

namespace game
{
	// Functions

	WEAK symbol<void(int type, VariableUnion u)> AddRefToValue{0x5656E0};
	WEAK symbol<void(unsigned int id)> AddRefToObject{0x5655F0};
	WEAK symbol<unsigned int(unsigned int id)> AllocThread{0x565580};
	WEAK symbol<void(int type, VariableUnion u)> RemoveRefToValue{0x565730};
	WEAK symbol<void(unsigned int id)> RemoveRefToObject{0x5681E0};

	WEAK symbol<void(unsigned int weapon, bool isAlternate, char* output, unsigned int maxStringLen)> BG_GetWeaponNameComplete{0x42F760};

	WEAK symbol<const char*(int index)> ConcatArgs{0x502150};
	WEAK symbol<void(int localClientNum, const char* text)> Cbuf_AddText{0x545680};
	WEAK symbol<void(const char* cmdName, void(), cmd_function_t* allocedCmd)> Cmd_AddCommandInternal{0x545DF0};
	WEAK symbol<void(const char* cmdName)> Cmd_RemoveCommand{0x545E20};
	WEAK symbol<const char*(int index)> Cmd_Argv{0x467600};

	WEAK symbol<const dvar_t*(const char*)> Dvar_FindVar{0x5BDCC0};

	WEAK symbol<char*(const char*)> I_CleanStr{0x0};

	WEAK symbol<VariableValue(unsigned int classnum, int entnum, int offset)> GetEntityFieldValue{0x56AF20};
	WEAK symbol<unsigned int(unsigned int parentId, unsigned int name)> FindVariable{0x5651F0};
	WEAK symbol<unsigned int(unsigned int parentId, unsigned int name)> FindObject{0x565BD0};
	WEAK symbol<unsigned int(unsigned int parentId, unsigned int name)> GetVariable{0x5663E0};

	WEAK symbol<int(const char* cmd)> Key_GetBindingForCmd{0x48C1C0};

	WEAK symbol<const float*(const float* v)> Scr_AllocVector{0x565680};
	WEAK symbol<void()> Scr_ClearOutParams{0x569010};
	WEAK symbol<scr_entref_t(unsigned int entId)> Scr_GetEntityIdRef{0x565F60};
	WEAK symbol<void(unsigned int classnum, int entnum, int offset)> Scr_SetObjectField{0x52BCC0};
	WEAK symbol<void(int id, unsigned int stringValue, unsigned int paramcount)> Scr_NotifyId{0x56B5E0};
	WEAK symbol<int(const char* filename, unsigned int str)> Scr_GetFunctionHandle{0x5618A0};
	WEAK symbol<unsigned int(int handle, unsigned int objId, unsigned int paramcount)> Scr_ExecThreadInternal{0x56E1C0};

	WEAK symbol<unsigned int(const char* str, unsigned int user)> SL_GetString{0x5649E0};
	WEAK symbol<const char*(unsigned int stringValue)> SL_ConvertToString{0x564270};

	WEAK symbol<void(int clientNum, int type, const char* command)> SV_GameSendServerCommand{0x573220};
	WEAK symbol<void(int arg, char* buffer, int bufferLength)> SV_Cmd_ArgvBuffer{0x5459F0};

	WEAK symbol<void(unsigned int notifyListOwnerId, unsigned int stringValue, VariableValue* top)> VM_Notify{0x569720};
	WEAK symbol<unsigned int(unsigned int localId, unsigned int pos, unsigned int paramcount)> VM_Execute{0x56DFE0};

	WEAK symbol<void*(jmp_buf* Buf, int Value)> longjmp{0x7363BC};
	WEAK symbol<int(jmp_buf* Buf)> _setjmp{0x734CF8};

	// Variables

	WEAK symbol<CmdArgs> cmd_args{0x1C978D0};

	WEAK symbol<int> g_script_error_level{0x20B21FC};
	WEAK symbol<jmp_buf> g_script_error{0x20B4218};

	WEAK symbol<scrVmPub_t> scr_VmPub{0x20B4A80};
	WEAK symbol<scrVarGlob_t> scr_VarGlob{0x1E72180};
	WEAK symbol<function_stack_t> scr_function_stack{0x20B4A5C};

	WEAK symbol<scr_classStruct_t*> g_classMap{0x8B4300};

	WEAK symbol<gentity_s> g_entities{0x1A66E28};
	WEAK symbol<unsigned int> levelEntityId{0x208E1A4};
	WEAK symbol<unsigned int> gameEntityId{0x208E1A8};
}