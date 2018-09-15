#ifndef __PARSECMDLINE__
#define __PARSECMDLINE__

#include <string>
#include <map>
#include <unistd.h>

namespace parCmdLine{

	struct ParseResult{
		bool          hasValue,
		              isPresent;
		std::string   value;
	};

	class ParseCmdLine{
		public:
		    ParseCmdLine(int argc, char **argv, const char* flags);
		            bool         isSet(char flag)                           const noexcept;
		            bool         isLegal(char flag)                         const noexcept;
		            bool         hasValue(char flag)                        const noexcept;
		            bool         hasUnflaggedPars(void)                     const noexcept;
		    const   std::string& getUnflaggedPars(void)                     const noexcept;
		    const   std::string& getValue(char flag)                        const noexcept;
		            bool         getErrorState()                            const noexcept;

		private:
		    mutable bool                         errState;
					bool                         unflaggedParams;
		            int                          argcRef;
					std::map<char, ParseResult>  flagsStatus;
			const   std::string                  errString;
			        std::string                  unflaggedArgs;

		    bool          setErrorState(bool state)                          const noexcept;
		    bool          parseArgs(char **argv, const char* flags)                noexcept;
		    bool          tokenizeFlags(const char* flags)                         noexcept;
		    bool          setOn(char flag)                                         noexcept;
	};

}

#endif
