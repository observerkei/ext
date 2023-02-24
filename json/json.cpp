#include <cctype>
#include <new>
#include <vector>
#include <string>
#include <initializer_list>
#include <fstream>
#include <cstring>
#include "json.h"

class json {
    public:
        typedef enum { NUM, STR, BOL, ARR, OBJ, NUL, } type_t;
        typedef std::vector<json *> json_arr_t;
        typedef std::vector<json *> json_obj_t;

    public:
        json(type_t type):
            m_type(type), m_key("")
    {}
        json(const std::string &key): 
            m_type(NUL), m_key(key) 
    {}
        json(const std::string &key, const double      &num): 
            m_type(NUM), m_key(key), m_num(num) 
    {}
        json(const std::string &key, const int         &num): 
            m_type(NUM), m_key(key), m_num(num) 
    {}
        json(const std::string &key, const std::string &str): 
            m_type(STR), m_key(key), m_str(str) 
    {}
        json(const std::string &key, const bool        &bol): 
            m_type(BOL), m_key(key), m_bol(bol) 
    {}
        json(const std::string &key, std::initializer_list<double> li):
            m_type(ARR), m_key(key), m_arr()
    { for (auto num: li) m_arr.push_back(new(std::nothrow) json("", num)); }

        json(const std::string &key, std::initializer_list<int> li):
            m_type(ARR), m_key(key), m_arr()
    { for (auto num: li) m_arr.push_back(new(std::nothrow) json("", num)); }

        json(const std::string &key, std::initializer_list<const std::string> li):
            m_type(ARR), m_key(key), m_arr()
    { for (auto str: li) m_arr.push_back(new(std::nothrow) json("", str)); }

        json(const std::string &key, std::initializer_list<json *> li):
            m_type(OBJ), m_key(key), m_obj(li)
    {}
        ~json();

        std::string to_str();
        static json *load_str(const std::string &str);
        int to_file(const char *file_name);
        static json *load_file(const char *file_name);

        type_t type()
        { return m_type; }
        void set_key(const std::string &key)
        { m_key = key; }
        double num()
        { return NUM == m_type ? m_num : 0.0; }
        std::string str()
        { return STR == m_type ? m_str : NULL; }
        bool bol()
        { return BOL == m_type ? m_bol : false; }
        std::string key()
        { return m_key; }
        json_arr_t *arr()
        { return ARR == m_type ? &m_arr : nullptr; }
        json_obj_t *obj()
        { return OBJ == m_type ? &m_obj : nullptr; }

        json *operator[] (const std::string key)
        { 
            if (OBJ != m_type) 
                return nullptr;
            for (auto obj: m_obj) 
                if (key == obj->m_key) 
                    return obj;
            json *add = new(std::nothrow) json(key);
            if (!add) 
                return nullptr;
            m_obj.push_back(add);
            return add;
        }

    private:
        type_t m_type;
        std::string m_key;
        union {
            double m_num;
            std::string m_str;
            bool m_bol;
            json_arr_t m_arr;
            json_obj_t m_obj;
        };
};

json::~json()
{
    if (OBJ == m_type) {
        for (auto obj: m_obj)
            delete obj;
        m_obj.clear();
    }
    if (ARR == m_type) {
        for (auto arr: m_arr) 
            delete arr;
        m_arr.clear();
    }
}

std::string json::to_str()
{
    std::string out_str;
    bool is_first = true;

    if (!m_key.empty())
        out_str = "\"" + m_key + "\": ";

    switch (m_type) {
        case NUL:
            out_str += "null";
            break;
        case NUM:
            out_str += std::to_string(num());
            break;
        case STR:
            out_str += str();
            break;
        case BOL:
            out_str += (bol() ? "true" : "false");
            break;
        case ARR:
            out_str += "[ ";
            if (!arr()) {
                out_str += " ]";
                break;
            }
            for (const auto &iter: *arr()) {
                if (!iter)
                    continue;
                if (!is_first)
                    out_str += ", ";
                out_str += iter->to_str();
                is_first = false;
            }
            out_str += " ]";
            break;
        case OBJ:
            out_str += "{ ";
            if (!obj()) {
                out_str += " }";
                break;
            }
            for (const auto &iter: *obj()) {
                if (!iter)
                    continue;
                if (!is_first)
                    out_str += ", ";
                out_str += iter->to_str();
                is_first = false;
            }
            out_str += " }";
            break;
        default:
            break;
    }

    return out_str;
}

json *load_str_arr(const std::string &str, size_t &offset);
json *load_str_obj(const std::string &str, size_t &offset);

json *load_str_key_val(const std::string &str, size_t &offset)
{
    std::string key;
    std::string val;
    bool bol = false;
    json::type_t type;
    double num;

    if ('"' != str.at(offset))
        return nullptr;
    ++offset;

    for (; offset < str.length(); ++offset) {
        if ('"' == str.at(offset))
            break;
        key += str.at(offset);
    }
    if ('"' != str.at(offset))
        return nullptr;
    // skip "
    ++offset;

    if (key.empty())
        return nullptr;
    if (offset >= str.length())
        return nullptr;
    if ('[' == str.at(offset)) {
        json *arr = load_str_arr(str, offset);
        if (!arr)
            return nullptr;
        arr->set_key(key);
        return arr;
    }
    else if ('{' == str.at(offset)) {
        json *obj = load_str_obj(str, offset);
        if (!obj)
            return nullptr;
        obj->set_key(key);
        return new(std::nothrow) json(key.c_str(), obj);
    }

    for (; offset < str.length(); ++offset) {
        if (',' == str.at(offset) ||
            ']' == str.at(offset) ||
            '}' == str.at(offset))
            break;
        val += str.at(offset);
    }
    if (val.empty())
        return nullptr;

    if (val.length() > 2 &&
        '"' == *val.begin() &&
        '"' == *(val.end()-1)) {
        type = json::type_t::STR;
        val = val.substr(1, val.length()-2);
    } else if ((val.length() == (sizeof("true")-1)) &&
            !strcasecmp(val.c_str(), "true")) {
        type = json::type_t::BOL;
        bol = true;
    } else if ((val.length() == (sizeof("false")-1)) &&
            !strcasecmp(val.c_str(), "false")) {
        type = json::type_t::BOL;
        bol = false;
    } else if ((val.length() == (sizeof("null")-1)) &&
            !strcasecmp(val.c_str(), "null")) {
        type = json::type_t::NUL;
        (void)val;
    } else {
        for (const auto &ch: val)
            if (!std::isdigit(ch))
                return nullptr;
        type = json::type_t::NUM;
        num = std::stod(str);
    }

    json *js = nullptr;
    switch (type) {
        case json::type_t::NUM:
            js = new(std::nothrow) json(key.c_str(), num);
            if (!js)
                return nullptr;
            break;
        case json::type_t::STR:
            js = new(std::nothrow) json(key.c_str(), key.c_str());
            if (!js)
                return nullptr;
            break;
        case json::type_t::BOL:
            js = new(std::nothrow) json(key.c_str(), bol);
            if (!js)
                return nullptr;
            break;
        default:
            return nullptr;
    }

    return js;
}

json *load_str_arr(const std::string &str, size_t &offset)
{
    json *arr = nullptr;
    // skip [
    ++offset;
    if ('"' == str.at(offset)) {
        return load_str_key_val(str, offset);
    } else if ('{' == str.at(offset)) {
        return load_str_obj(str, offset);
    } else if ('[' == str.at(offset)) {
        ++offset;
        arr = new(std::nothrow) json(json::type_t::ARR);
        if (!arr)
            return nullptr;

        for (; offset < str.length();) {
            if (']' == str.at(offset)) {
                ++offset;
                break;
            } 
            json *iter = nullptr;
            // parser json obj
            if ('{' == str.at(offset)) {
                iter = load_str_obj(str, offset);
                if (!iter)
                    goto err;
            }
            // parser json arr
            else if ('[' == str.at(offset)) {
                iter = load_str_arr(str, offset);
                if (!iter)
                    goto err;
            }
            // parser json key
            else if ('"' == str.at(offset)) {
                for (size_t check = offset; check < str.length(); ++check) {
                    if ('"' == str.at(check))
                        continue;
                    size_t str_len = check - offset;
                    iter = new(std::nothrow) json(str.substr(offset, str_len));
                    if (!iter)
                        goto err;
                    offset += str_len;
                }
            }
            // parser json num
            else if (std::isdigit(str.at(offset))) {
                for (size_t check = offset; check < str.length();++check) {
                    if (!std::isdigit(str.at(check)) && 
                            ('.' != str.at(check)) &&
                            (',' != str.at(check)) &&
                            (']' != str.at(check))) {
                        goto err;
                    }
                    if (',' != str.at(check) &&
                        ']' != str.at(check)) {
                        continue;
                    }
                    size_t num_len = check - offset;
                    if (!num_len)
                        goto err;
                    iter = new(std::nothrow) json("", std::stod(str.substr(offset, num_len)));
                    if (!iter)
                        goto err;
                    offset += num_len;
                    break;
                }
            }
            // parser json bol
            else {
                size_t less_str = str.length() - offset;
                if ((less_str > sizeof("true") - 1) &&
                    !strcasecmp("true", str.substr(offset, sizeof("true")-1).c_str())) {
                    iter = new(std::nothrow) json("", true);
                    if (!iter)
                        goto err;
                    offset += sizeof("true")-1;
                }
                else if ((less_str > sizeof("false") - 1) &&
                        !strcasecmp("false", str.substr(offset, sizeof("fasle")-1).c_str())) {
                    iter = new(std::nothrow) json("", false);
                    if (!iter)
                        goto err;
                    offset += sizeof("false")-1;
                }
            }

            if (!iter)
                goto err;
            if (!arr->arr()->empty() &&
                (*arr->arr()->begin())->type() != iter->type()) 
                goto err;
            arr->arr()->push_back(iter);

            if (',' == str.at(offset)) {
                ++offset;
                continue;
            }

            goto err;
        }
    }
    return arr;
err:
    if (arr)
        delete arr;
    return nullptr;
}

json *load_str_obj(const std::string &str, size_t &offset)
{
    if ('{' != str.at(offset))
        return nullptr;

    json *obj = new(std::nothrow) json(json::type_t::OBJ);
    if (!obj)
        return nullptr;

    for (; offset < str.length();) {
        json *iter = nullptr;
        if ('"' == str.at(offset)) {
            iter = load_str_key_val(str, offset);
            if (!iter) 
                goto err;
        }
        if (!iter)
            goto err;
        obj->obj()->push_back(iter);
        if ('}' == str.at(offset)) 
            break;
        if (',' == str.at(offset)) {
            ++offset;
            continue;
        }
    }
    if ('}' != str.at(offset)) {
        goto err;
    }
    // skip }
    ++offset;

    return obj;
err:
    if (obj)
        delete obj;
    return nullptr;
}

json *json::load_str(const std::string &str)
{
    std::string parser;
    bool left = false;
    bool right = false;

    for (const auto &ch: str) {
        if (left && '"' == ch)
            right = true;
        if (!right && '"' == ch)
            left = true;

        if (!left && ' ' != ch)
            parser += ch;

        if (left && right) {
            left = false;
            right = false;
        }
    }
    if (parser.empty())
        return nullptr;

    size_t offset = 0;
    if ('{' == *parser.begin())
        return load_str_obj(parser, offset);
    else if ('[' == *parser.begin())
        return load_str_obj(parser, offset);

    return nullptr;
}

int json::to_file(const char *file_name)
{
    if (!file_name)
        return -1;

    std::ofstream outfile(file_name, std::ios::out | std::ios::trunc );

    outfile << to_str();

    outfile.close();

    return 0;
}

json *json::load_file(const char *file_name)
{
    if (!file_name)
        return nullptr;
    std::ifstream infile(file_name, std::ios::in);

    std::string instr;
    infile >> instr;

    json *load_json = json::load_str(instr);

    infile.close();

    return load_json;
}

#ifndef main

#include <iostream>
using namespace std;

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    const char *input = argv[1];

    json jnul("jnul");
    json js("js", 100);
    json jstr("jstr", "hi!");
    json jbol("jbol", true);
    json jarr_str("", { "1", "2", "3"});
    json jobj("", { new json("A", 90), new json("B", false)});
    json jarr_num("", { 1, 2, 3});
    json root("", { 
            new json("A", { 
                    new json("a", 1),
                    new json("b", 2)
                    }),
            new json("B", { 1, 2, 3}),
            new json("C")
            });

    cout << js.to_str() << "\n"
        << jstr.to_str() << "\n" 
        << jbol.to_str() << "\n"
        << jobj.to_str() << "\n"
        << jarr_num.to_str() << "\n"
        << jarr_str.to_str() << "\n"
        << root.to_str() << "\n"
        << root["B"]->to_str() << "\n"
        << "\n";
    cout << "out > 1.js" << "\n";

    root.to_file("1.js");
    if (input) {
        cout << "read input: " << input << "\n";

        json *js = json::load_file(input);
        if (!js)
            return -1;

        cout << "read file 1.js: " << "\n" 
            << js->to_str() << "\n";

        delete js;
    }

    return 0;
}

#endif
