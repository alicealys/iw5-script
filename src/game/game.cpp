#include <stdinc.hpp>

namespace game
{
	void Scr_ClearOutParams()
	{
		const auto num_params = scr_VmPub->outparamcount;
		for (unsigned int i = num_params; i > 0; --i)
		{
			const auto value = scr_VmPub->top[i - 1];
			RemoveRefToValue(value.type, value.u);
		}

		scr_VmPub->top -= num_params;
	}
}
