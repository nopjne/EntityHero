#include <rapidjson.h>
#include <document.h>     // rapidjson's DOM-style API
#include <prettywriter.h> // for stringify JSON
#include <filereadstream.h>
#include <windows.h>
#include <istreamwrapper.h>
#include <ostreamwrapper.h>
#include <prettywriter.h>
#include <fstream>
#include <cstdio>

using namespace rapidjson;
using namespace std;

// This is just for quick testing the entity parser.
int UnitTest(void)
{
    ifstream InputStream("../UnitTest/testinput1.txt");
    if (InputStream.good() == false) {
        return 0;
    }

    rapidjson::IStreamWrapper InStream(InputStream);
    Document document;
    document.ParseStream<rapidjson::kParseCommentsFlag | rapidjson::kParseTrailingCommasFlag | rapidjson::kParseNanAndInfFlag | rapidjson::kIgnoreWhiteSpacing>(InStream);
    if (document.HasParseError() != false) {
        char string[1024];
        sprintf_s(string, "bad input file %zu\n", document.GetErrorOffset());
        OutputDebugStringA(string);
        printf(string);
    }

    ofstream OfStream("../UnitTest/testoutput1.txt", std::ofstream::binary);
    if (OfStream.good() == false) {
        return 0;
    }
    rapidjson::OStreamWrapper osw(OfStream);
    rapidjson::PrettyWriter<rapidjson::OStreamWrapper,
        rapidjson::UTF8<char>,
        rapidjson::UTF8<char>,
        rapidjson::CrtAllocator,
        rapidjson::kWriteValidateEncodingFlag |
        rapidjson::kWriteNanAndInfFlag |
        rapidjson::kWriteKeepNumForNullArray> writer(osw);
    writer.SetIndent('\t', 1);
    //rapidjson::OStreamWrapper OutStream(OfStream);
    document.Accept(writer, 0);
    OfStream.close();

    size_t offset = 0;
    size_t line = 1;
    size_t col = 0;
    FILE *F1, *F2;
    fopen_s(&F1, "../UnitTest/testoutput1.txt", "rb");
    fopen_s(&F2, "../UnitTest/testverification1.txt", "rb");
    while ((feof(F1) == false) && (feof(F2) == false)) {
        char inp1, inp2;
        fread(&inp1, 1, 1, F1);
        fread(&inp2, 1, 1, F2);

        if (inp1 == '\n') {
            line += 1;
            col = offset - 3;
        }

        if (inp1 != inp2) {
            char strtest[1024];
            sprintf_s(strtest, "Mismatch occured at offset:%llu line:%llu col:%llu", offset, line, (offset - col));
            MessageBoxA(NULL, strtest, "UnitTest failed", MB_OK | MB_ICONERROR);
            break;
        }

        offset += 1;
    }

    return 0;
}