
using System.IO;

class AutoCodingClass
{
    static string filePath = "..//..//..//Message.idl";
    static string headerPath = "..//..//..//Message.h";
    static string cppPath = "..//..//..//Message.cpp";

    static List<string> procHeaderList = new List<string>();
    static List<string> imPlementPart = new List<string>();

    static StreamWriter headerWriter = File.CreateText(headerPath);
    static StreamWriter cppWriter = File.CreateText(cppPath);

    static void CreateHeaderFile()
    {
        for (int i = 0; i < procHeaderList.Count; i++)
        {
            procHeaderList[i] = string.Format($"{procHeaderList[i]};");
            headerWriter.WriteLine(procHeaderList[i]);
        }
    }

    static void CreatePacketHeader(string proc, int iType, int size)
    {
        // TODO: type을 매핑시켜서 문자열로 넣기. 쌈@뽕하게

        int idx1 = proc.IndexOf('(');
        int idx2 = proc.IndexOf(')');
        string tmp = proc.Substring(idx1 + 1, (idx2 - 1) - idx1);
        List<string> paramStrList = new List<string>();

        /* sPacket 인자 추출 */
        string[] paramsLineStr = tmp.Split(", ");
        string[] tmpParamStr = paramsLineStr[0].Split(" ");

        string sPacketStr = tmpParamStr[1];
        sPacketStr = string.Format($"(*{sPacketStr})");


        /* 패킷 헤더 선언 */
        string stHeaderStr = "stPacketHeader";
        string headerVal = "header";
        string code = "0x89";

        string str0 = string.Format($"\t{stHeaderStr} {headerVal};");
        string str1 = string.Format($"\t{headerVal}._code = {code};");
        string str2 = string.Format($"\t{headerVal}._size = {size};");
        string str3 = string.Format($"\t{headerVal}._type = {iType};");
        
        imPlementPart.Add(str0);
        imPlementPart.Add(str1);
        imPlementPart.Add(str2);
        imPlementPart.Add(str3);

        string str4 = string.Format($"\t{sPacketStr}.Putdata((char*)&{headerVal}, sizeof({stHeaderStr}));");

        imPlementPart.Add(str4);

    }

    static void mpCreateMyCharacter(string proc, int iType)
    {
        int size = 10;

        CreatePacketHeader(proc, iType, size);

        int idx1 = proc.IndexOf('(');
        int idx2 = proc.IndexOf(')');
        string tmp = proc.Substring(idx1 + 1, (idx2 - 1) - idx1);

        List<string> paramStrList = new List<string>();

        /* 인자 추출 */
        string[] paramsLineStr = tmp.Split(", ");
        for(int i = 0; i < paramsLineStr.Length; i++)
        {
            string[] tmpParamStr = paramsLineStr[i].Split(" ");

            for(int j=0; j < tmpParamStr.Length; j++)
            {
                if (j % 2 == 0)
                    continue;

                paramStrList.Add(tmpParamStr[j]);
            }
        }

        /* 직렬화 버퍼에 넣기 */
        // 직렬화 버퍼는 항상 parameStrList의 첫번째입니다.
        
        // ex) (*sPacket) << dir;
        string str1 = String.Format($"(*{paramStrList[0]}) << ");
        for (int i = 1; i < paramStrList.Count; i++)
        {
            string str2 = String.Format($"\t{str1} {paramStrList[i]};");
            imPlementPart.Add(str2);
        }

    }

    static void mpCreateOtherCharacter(string proc, int iType)
    {
        int size = 10;

        CreatePacketHeader(proc, iType, size);

        int idx1 = proc.IndexOf('(');
        int idx2 = proc.IndexOf(')');
        string tmp = proc.Substring(idx1 + 1, (idx2 - 1) - idx1);

        List<string> paramStrList = new List<string>();

        /* 인자 추출 */
        string[] paramsLineStr = tmp.Split(", ");
        for (int i = 0; i < paramsLineStr.Length; i++)
        {
            string[] tmpParamStr = paramsLineStr[i].Split(" ");

            for (int j = 0; j < tmpParamStr.Length; j++)
            {
                if (j % 2 == 0)
                    continue;

                paramStrList.Add(tmpParamStr[j]);
            }
        }

        /* 직렬화 버퍼에 넣기 */
        // 직렬화 버퍼는 항상 parameStrList의 첫번째입니다.

        // ex) (*sPacket) << dir;
        string str1 = String.Format($"(*{paramStrList[0]}) << ");
        for (int i = 1; i < paramStrList.Count; i++)
        {
            string str2 = String.Format($"\t{str1} {paramStrList[i]};");
            imPlementPart.Add(str2);
        }
    }

    static void mpDeleteCharacter(string proc, int iType)
    {
        int size = 4;

        CreatePacketHeader(proc, iType, size);

        int idx1 = proc.IndexOf('(');
        int idx2 = proc.IndexOf(')');
        string tmp = proc.Substring(idx1 + 1, (idx2 - 1) - idx1);

        List<string> paramStrList = new List<string>();

        /* 인자 추출 */
        string[] paramsLineStr = tmp.Split(", ");
        for (int i = 0; i < paramsLineStr.Length; i++)
        {
            string[] tmpParamStr = paramsLineStr[i].Split(" ");

            for (int j = 0; j < tmpParamStr.Length; j++)
            {
                if (j % 2 == 0)
                    continue;

                paramStrList.Add(tmpParamStr[j]);
            }
        }

        /* 직렬화 버퍼에 넣기 */
        // 직렬화 버퍼는 항상 parameStrList의 첫번째입니다.

        // ex) (*sPacket) << dir;
        string str1 = String.Format($"(*{paramStrList[0]}) << ");
        for (int i = 1; i < paramStrList.Count; i++)
        {
            string str2 = String.Format($"\t{str1} {paramStrList[i]};");
            imPlementPart.Add(str2);
        }
    }

    static void mpMoveStart(string proc, int iType)
    {
        int size = 9;

        CreatePacketHeader(proc, iType, size);

        int idx1 = proc.IndexOf('(');
        int idx2 = proc.IndexOf(')');
        string tmp = proc.Substring(idx1 + 1, (idx2 - 1) - idx1);

        List<string> paramStrList = new List<string>();

        /* 인자 추출 */
        string[] paramsLineStr = tmp.Split(", ");
        for (int i = 0; i < paramsLineStr.Length; i++)
        {
            string[] tmpParamStr = paramsLineStr[i].Split(" ");

            for (int j = 0; j < tmpParamStr.Length; j++)
            {
                if (j % 2 == 0)
                    continue;

                paramStrList.Add(tmpParamStr[j]);
            }
        }

        /* 직렬화 버퍼에 넣기 */
        // 직렬화 버퍼는 항상 parameStrList의 첫번째입니다.

        // ex) (*sPacket) << dir;
        string str1 = String.Format($"(*{paramStrList[0]}) << ");
        for (int i = 1; i < paramStrList.Count; i++)
        {
            string str2 = String.Format($"\t{str1} {paramStrList[i]};");
            imPlementPart.Add(str2);
        }
    }

    static void mpMoveStop(string proc, int iType)
    {
        int size = 9;

        CreatePacketHeader(proc, iType, size);

        int idx1 = proc.IndexOf('(');
        int idx2 = proc.IndexOf(')');
        string tmp = proc.Substring(idx1 + 1, (idx2 - 1) - idx1);

        List<string> paramStrList = new List<string>();

        /* 인자 추출 */
        string[] paramsLineStr = tmp.Split(", ");
        for (int i = 0; i < paramsLineStr.Length; i++)
        {
            string[] tmpParamStr = paramsLineStr[i].Split(" ");

            for (int j = 0; j < tmpParamStr.Length; j++)
            {
                if (j % 2 == 0)
                    continue;

                paramStrList.Add(tmpParamStr[j]);
            }
        }

        /* 직렬화 버퍼에 넣기 */
        // 직렬화 버퍼는 항상 parameStrList의 첫번째입니다.

        // ex) (*sPacket) << dir;
        string str1 = String.Format($"(*{paramStrList[0]}) << ");
        for (int i = 1; i < paramStrList.Count; i++)
        {
            string str2 = String.Format($"\t{str1} {paramStrList[i]};");
            imPlementPart.Add(str2);
        }
    }

    static void mpAttack1(string proc, int iType)
    {
        int size = 9;

        CreatePacketHeader(proc, iType, size);

        int idx1 = proc.IndexOf('(');
        int idx2 = proc.IndexOf(')');
        string tmp = proc.Substring(idx1 + 1, (idx2 - 1) - idx1);

        List<string> paramStrList = new List<string>();

        /* 인자 추출 */
        string[] paramsLineStr = tmp.Split(", ");
        for (int i = 0; i < paramsLineStr.Length; i++)
        {
            string[] tmpParamStr = paramsLineStr[i].Split(" ");

            for (int j = 0; j < tmpParamStr.Length; j++)
            {
                if (j % 2 == 0)
                    continue;

                paramStrList.Add(tmpParamStr[j]);
            }
        }

        /* 직렬화 버퍼에 넣기 */
        // 직렬화 버퍼는 항상 parameStrList의 첫번째입니다.

        // ex) (*sPacket) << dir;
        string str1 = String.Format($"(*{paramStrList[0]}) << ");
        for (int i = 1; i < paramStrList.Count; i++)
        {
            string str2 = String.Format($"\t{str1} {paramStrList[i]};");
            imPlementPart.Add(str2);
        }
    }

    static void mpAttack2(string proc, int iType)
    {
        int size = 9;

        CreatePacketHeader(proc, iType, size);

        int idx1 = proc.IndexOf('(');
        int idx2 = proc.IndexOf(')');
        string tmp = proc.Substring(idx1 + 1, (idx2 - 1) - idx1);

        List<string> paramStrList = new List<string>();

        /* 인자 추출 */
        string[] paramsLineStr = tmp.Split(", ");
        for (int i = 0; i < paramsLineStr.Length; i++)
        {
            string[] tmpParamStr = paramsLineStr[i].Split(" ");

            for (int j = 0; j < tmpParamStr.Length; j++)
            {
                if (j % 2 == 0)
                    continue;

                paramStrList.Add(tmpParamStr[j]);
            }
        }

        /* 직렬화 버퍼에 넣기 */
        // 직렬화 버퍼는 항상 parameStrList의 첫번째입니다.

        // ex) (*sPacket) << dir;
        string str1 = String.Format($"(*{paramStrList[0]}) << ");
        for (int i = 1; i < paramStrList.Count; i++)
        {
            string str2 = String.Format($"\t{str1} {paramStrList[i]};");
            imPlementPart.Add(str2);
        }
    }

    static void mpAttack3(string proc, int iType)
    {
        int size = 9;

        CreatePacketHeader(proc, iType, size);

        int idx1 = proc.IndexOf('(');
        int idx2 = proc.IndexOf(')');
        string tmp = proc.Substring(idx1 + 1, (idx2 - 1) - idx1);

        List<string> paramStrList = new List<string>();

        /* 인자 추출 */
        string[] paramsLineStr = tmp.Split(", ");
        for (int i = 0; i < paramsLineStr.Length; i++)
        {
            string[] tmpParamStr = paramsLineStr[i].Split(" ");

            for (int j = 0; j < tmpParamStr.Length; j++)
            {
                if (j % 2 == 0)
                    continue;

                paramStrList.Add(tmpParamStr[j]);
            }
        }

        /* 직렬화 버퍼에 넣기 */
        // 직렬화 버퍼는 항상 parameStrList의 첫번째입니다.

        // ex) (*sPacket) << dir;
        string str1 = String.Format($"(*{paramStrList[0]}) << ");
        for (int i = 1; i < paramStrList.Count; i++)
        {
            string str2 = String.Format($"\t{str1} {paramStrList[i]};");
            imPlementPart.Add(str2);
        }
    }

    static void mpDamage(string proc, int iType)
    {
        int size = 9;

        CreatePacketHeader(proc, iType, size);

        int idx1 = proc.IndexOf('(');
        int idx2 = proc.IndexOf(')');
        string tmp = proc.Substring(idx1 + 1, (idx2 - 1) - idx1);

        List<string> paramStrList = new List<string>();

        /* 인자 추출 */
        string[] paramsLineStr = tmp.Split(", ");
        for (int i = 0; i < paramsLineStr.Length; i++)
        {
            string[] tmpParamStr = paramsLineStr[i].Split(" ");

            for (int j = 0; j < tmpParamStr.Length; j++)
            {
                if (j % 2 == 0)
                    continue;

                paramStrList.Add(tmpParamStr[j]);
            }
        }

        /* 직렬화 버퍼에 넣기 */
        // 직렬화 버퍼는 항상 parameStrList의 첫번째입니다.

        // ex) (*sPacket) << dir;
        string str1 = String.Format($"(*{paramStrList[0]}) << ");
        for (int i = 1; i < paramStrList.Count; i++)
        {
            string str2 = String.Format($"\t{str1} {paramStrList[i]};");
            imPlementPart.Add(str2);
        }
    }

    static void CreateCPPFile(string proc, string type)
    {
        imPlementPart.Clear();

        imPlementPart.Add(proc);
        imPlementPart.Add("{");

        int iType = Convert.ToInt32(type);
        switch(iType)
        {
            case 0:
                mpCreateMyCharacter(proc, iType);
                break;
            case 1:
                mpCreateOtherCharacter(proc, iType);
                break;
            case 2:
                mpDeleteCharacter(proc, iType);
                break;
            case 11:
                mpMoveStart(proc, iType);
                break;
            case 13:
                mpMoveStop(proc, iType);
                break;
            case 21:
                mpAttack1(proc, iType);
                break;
            case 23:
                mpAttack2(proc, iType);
                break;
            case 25:
                mpAttack3(proc, iType);
                break;
            case 30:
                mpDamage(proc, iType);
                break;
        }


        imPlementPart.Add("}");

        for(int i = 0;i < imPlementPart.Count;i++)
        {
            cppWriter.WriteLine(imPlementPart[i]);
        }
        cppWriter.WriteLine(" ");
    }

    static void CreateInclude()
    {
        // cpp 파일 맨 윗단 include 파트
        string[] includeStr = { "Windows.h", "SerializeBuffer.h", "Message.h", "PacketData.h" };
        for (int j = 0; j < includeStr.Length; j++)
        {
            string tmp = string.Format($"#include \"{includeStr[j]}\"");
            imPlementPart.Add(tmp);
        }
        for (int i = 0; i < imPlementPart.Count; i++)
        {
            cppWriter.WriteLine(imPlementPart[i]);
        }
        cppWriter.WriteLine(" ");
    }

    static void Main(string[] args)
    {
        // .cpp 맨 위include 파트
        CreateInclude();

        // .cpp 함수 정의 파트
        var lines = File.ReadAllLines(filePath);
        for(int i=0; i<lines.Length; i++)
        {
            var line = lines[i];

            if (String.IsNullOrEmpty(line))
                continue;

            string[] str = line.Split(':');

            procHeaderList.Add(str[0]);

            CreateCPPFile(str[0], str[1]);
        }

        // .h 함수 선언 파트
        CreateHeaderFile();


        headerWriter.Close();
        cppWriter.Close();
    }

}