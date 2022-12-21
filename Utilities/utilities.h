using namespace std;

string bufToString(char buffer[], int len)
{
    string ans = "";
    for (int i = 0; i < len; i++)
        ans.push_back(buffer[i]);
    return ans;
}

string getType(string request)
{
    return request.substr(0, request.find("\r\n"));
}

string getHeader(string request)
{
    return request.substr(0, request.find("\r\n\r\n"));
}

string getBody(string request)
{
    return request.substr(request.find("\r\n\r\n") + 4);
}

pair<char *, int> getFileInfo(string path)
{
    ifstream fr;
    fr.open(path, ios::binary); // read file as binaries
    if (fr.is_open() == false)
    {
        cout << "File not found" << endl;
        exit(1);
    }
    fr.seekg(0, ios::end);
    size_t sz = fr.tellg(); // get file size

    char *readFile = new char[sz];
    fr.seekg(0, ios::beg); // return pointer to begining of file to start reading
    fr.read(readFile, sz);
    fr.close();
    return {readFile, sz};
}

void writeFile(string path, string body)
{
    body.pop_back(); // remove \n
    body.pop_back(); // remove \r
    ofstream fw(path);
    fw.write(body.c_str(), body.size());
    fw.close();
}
