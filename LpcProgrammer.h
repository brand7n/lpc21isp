#pragma once

#include "lpc21isp.h"

namespace lpcprog
{

public ref class LpcProgrammer
{
public:
	LpcProgrammer(StringOutDelegate ^fp);
	int Program(System::String ^firmwareName);
private:
	int OpenFTDI(void);

    ISP_ENVIRONMENT IspEnvironment;
	StringOutDelegate ^myFp;

};
}
