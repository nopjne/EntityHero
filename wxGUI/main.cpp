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
int gArrayCount = 0;
int main(void)
{
    ifstream InputStream("../chrispy.txt");
    if (InputStream.good() == false) {
        return 0;
    }

    rapidjson::IStreamWrapper InStream(InputStream);
    Document document;
    document.ParseStream<rapidjson::kParseCommentsFlag | rapidjson::kParseTrailingCommasFlag | rapidjson::kParseNanAndInfFlag>(InStream);
    if (document.HasParseError() != false) {
        printf("bad input file\n");
    }

    ofstream OfStream("../chrispy_out.txt", std::ofstream::binary);
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

    return 0;
}