#include <cctype>
#include <new>
#include <sstream>
#include <vector>
#include <string>
#include <initializer_list>
#include <fstream>
#include <cstring>
#include <cassert>

#include "json.h"

#include <cstdio>
#define dmsg(fmt, ...) if (g_enable_json_dbg) fprintf(stdout, "[%s:%s:%d] " fmt "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__)
static bool g_enable_json_dbg = false;

class json {
    public:
        typedef enum { BOL, NUM, STR, ARR, OBJ, NUL, } type_t;
        typedef bool bol_t;
        typedef double num_t;
        typedef std::string str_t;
        typedef std::vector<json *> arr_t;
        typedef std::vector<json *> obj_t;

    public:
        json(type_t type):
            m_type(type), m_key("")
    {
        switch (type) {
            case BOL: m_bol = bol_t(); break;
            case NUM: m_num = num_t(); break;
            case STR: m_str = new(std::nothrow) str_t(); break;
            case ARR: m_arr = new(std::nothrow) arr_t(); break;
            case OBJ: m_obj = new(std::nothrow) obj_t(); break;
            case NUL: break;
            default: break;
        }
    }
        json(const str_t &key): 
            m_type(NUL), m_key(key) 
    {}
        json(const str_t &key, const bool        &bol): 
            m_type(BOL), m_key(key), m_bol(bol) 
    {}
        json(const str_t &key, const double      &num): 
            m_type(NUM), m_key(key), m_num(num) 
    {}
        json(const str_t &key, const int         &num): 
            m_type(NUM), m_key(key), m_num(num) 
    {}
        json(const str_t &key, const str_t &str): 
            m_type(STR), m_key(key), m_str(new(std::nothrow) str_t(str)) 
    {}
        json(const str_t &key, const std::initializer_list<double> &li):
            m_type(ARR), m_key(key), m_arr(new(std::nothrow) arr_t)
    { for (const auto &num: li) m_arr->push_back(new(std::nothrow) json("", num)); }

        json(const str_t &key, const std::initializer_list<int> &li):
            m_type(ARR), m_key(key), m_arr(new(std::nothrow) arr_t)
    { for (const auto &num: li) m_arr->push_back(new(std::nothrow) json("", num));}

        json(const str_t &key, const std::initializer_list<const str_t> &li):
            m_type(ARR), m_key(key), m_arr(new(std::nothrow) arr_t)
    { for (const auto &str: li) m_arr->push_back(new(std::nothrow) json("", str)); }

        json(const str_t &key, const std::initializer_list<json *> &li):
            m_type(OBJ), m_key(key), m_obj(new(std::nothrow) obj_t)
    { for (const auto &obj: li) m_obj->push_back(obj); }
        ~json();

        str_t to_str();
        int to_file(const char *file_name);
        static json *load_str(const str_t &str);
        static json *load_file(const char *file_name);

        bool is_num(const str_t &str) 
        {
            for (const auto &ch: str)
                if (!std::isdigit(ch) || '.' == ch)
                    return false;
            return true;
        }

        bool is_bol(const str_t &str) 
        {
            return (((str.length() == (sizeof("true")-1)) && 
                        !strcasecmp("true", str.c_str())) ||
                    ((str.length() == (sizeof("false")-1)) &&
                     !strcasecmp("false", str.c_str())));
        }

        type_t type()
        { return m_type; }
        void set_key(const str_t &key)
        { m_key = key; }
        double num()
        { return NUM == m_type ? m_num : 0.0; }
        str_t str()
        { return STR == m_type ? *m_str : NULL; }
        bool bol()
        { return BOL == m_type ? m_bol : false; }
        str_t key()
        { return m_key; }
        arr_t *arr()
        { return ARR == m_type ? m_arr : nullptr; }
        obj_t *obj()
        { return OBJ == m_type ? m_obj : nullptr; }

        json *operator[] (const str_t &key)
        { 
            if (OBJ != m_type) 
                return nullptr;
            for (auto obj: *m_obj) 
                if (key == obj->m_key) 
                    return obj;
            json *add = new(std::nothrow) json(key);
            if (!add) 
                return nullptr;
            m_obj->push_back(add);
            return add;
        }

    private:
        type_t m_type;
        str_t m_key;
        union {
            bol_t m_bol;
            num_t m_num;
            str_t *m_str;
            arr_t *m_arr;
            obj_t *m_obj;
        };
};

json::~json()
{
    //dmsg("type: %d\n", m_type);
    if (STR == m_type) {
        delete m_str;
        m_str = nullptr;
    }
    if (OBJ == m_type) {
        for (auto obj: *m_obj) {
            if (!obj) {
                continue;
            }
            // dmsg("del: %s\n", obj->to_str().c_str());

            delete obj;
        }
        m_obj->clear();
        m_obj = nullptr;
    }
    if (ARR == m_type) {
        for (auto iter: *m_arr) {
            if (!iter) {
                continue;
            }

            //dmsg("del: %s\n", (iter)->to_str().c_str());

            delete iter;
        } 
        m_arr->clear();
        m_arr = nullptr;
    }
}

std::string json::to_str()
{
    str_t out_str;
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
            out_str += '"' + str() + '"';
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
            dmsg("");
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
    dmsg("");

    for (; offset < str.length(); ++offset) {
        if ('"' == str.at(offset))
            break;
        key += str.at(offset);
    }
    dmsg("");
    if ('"' != str.at(offset))
        return nullptr;
    // skip "
    ++offset;

    // skip :
    ++offset;

    dmsg("");
    if (key.empty())
        return nullptr;
    dmsg("");
    if (offset >= str.length())
        return nullptr;
    dmsg("[->%c", str.at(offset));
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
        return obj;
    }

    dmsg("");
    for (; offset < str.length(); ++offset) {
        if (',' == str.at(offset) ||
                ']' == str.at(offset) ||
                '}' == str.at(offset))
            break;
        val += str.at(offset);
        dmsg("");
    }
    if (val.empty())
        return nullptr;

    dmsg("k:%s, v:%s", key.c_str(), val.c_str());
    if (val.length() > 2 &&
            '"' == *val.begin() &&
            '"' == *(val.end()-1)) {
        dmsg("");
        type = json::type_t::STR;
        val = val.substr(1, val.length()-2);
        dmsg("set val:%s", val.c_str());
    } else if ((val.length() == (sizeof("true")-1)) &&
            !strcasecmp(val.c_str(), "true")) {
        dmsg("");
        type = json::type_t::BOL;
        bol = true;
    } else if ((val.length() == (sizeof("false")-1)) &&
            !strcasecmp(val.c_str(), "false")) {
        dmsg("");
        type = json::type_t::BOL;
        bol = false;
    } else if ((val.length() == (sizeof("null")-1)) &&
            !strcasecmp(val.c_str(), "null")) {
        dmsg("");
        type = json::type_t::NUL;
        (void)val;
    } else {
        dmsg("");
        for (const auto &ch: val)
            if (!std::isdigit(ch) && '.' != ch)
                return nullptr;
        type = json::type_t::NUM;
        num = std::stod(val);
    }

    dmsg("type:%d", type);
    json *js = nullptr;
    switch (type) {
        case json::type_t::NUL:
            js = new(std::nothrow) json(key);
            if (!js)
                return nullptr;
            break;
        case json::type_t::NUM:
            js = new(std::nothrow) json(key, num);
            if (!js)
                return nullptr;
            break;
        case json::type_t::STR:
            dmsg("");
            js = new(std::nothrow) json(key, val);
            if (!js)
                return nullptr;
            break;
        case json::type_t::BOL:
            js = new(std::nothrow) json(key, bol);
            if (!js)
                return nullptr;
            break;
        default:
            return nullptr;
    }
    dmsg("create kv:%s", js->to_str().c_str());

    return js;
}

json *load_str_arr(const std::string &str, size_t &offset)
{
    json *arr = nullptr;
    dmsg("arr:instr:%c", str.at(offset));
    if ('[' != str.at(offset)) 
        return nullptr;
    dmsg("");

    // skip [
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
            dmsg("");
            iter = load_str_obj(str, offset);
            if (!iter)
                goto err;
        }
        // parser json arr
        else if ('[' == str.at(offset)) {
            dmsg("");
            iter = load_str_arr(str, offset);
            if (!iter)
                goto err;
        }
        // parser json key
        else if ('"' == str.at(offset)) {
            dmsg("");
            // skip "
            ++offset;
            for (size_t check = offset; check < str.length(); ++check) {
                dmsg("offset:%zu, str:%s", offset, str.substr(offset).c_str());
                if ('"' != str.at(check))
                    continue;
                size_t str_len = check - offset;
                iter = new(std::nothrow) json(str.substr(offset, str_len));
                if (!iter)
                    goto err;
                offset += str_len;
                //skip "
                ++offset;
                break;
            }
        }
        // parser json num
        else if (std::isdigit(str.at(offset))) {
            dmsg("");
            for (size_t check = offset; check < str.length();++check) {
                dmsg("offset:%zu, str:%s", offset, str.substr(offset).c_str());
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
                dmsg("offset:%zu, str:%s", offset, str.substr(offset).c_str());
                iter = new(std::nothrow) json("", std::stod(str.substr(offset, num_len)));
                if (!iter)
                    goto err;
                offset += num_len;
                break;
            }
        }
        // parser json bol
        else {
            dmsg("");
            size_t less_str = str.length() - offset;
            if ((less_str > sizeof("true") - 1) &&
                    !strcasecmp("true", str.substr(offset, sizeof("true")-1).c_str())) {
                dmsg("offset:%zu, str:%s", offset, str.substr(offset).c_str());
                iter = new(std::nothrow) json("", true);
                if (!iter)
                    goto err;
                offset += sizeof("true")-1;
            }
            else if ((less_str > sizeof("false") - 1) &&
                    !strcasecmp("false", str.substr(offset, sizeof("fasle")-1).c_str())) {
                dmsg("offset:%zu, str:%s", offset, str.substr(offset).c_str());
                iter = new(std::nothrow) json("", false);
                if (!iter)
                    goto err;
                offset += sizeof("false")-1;
            }
        }
        dmsg("bol done");

        if (!iter)
            goto err;
        dmsg("");
        if (!arr->arr()->empty() &&
                (*arr->arr()->begin())->type() != iter->type()) 
            goto err;
        dmsg("");
        arr->arr()->push_back(iter);

        if (',' == str.at(offset)) {
            ++offset;
            continue;
        }
        if (']' == str.at(offset)) {
            ++offset;
            break;
        }
        dmsg("offset:%zu, str:%s", offset, str.substr(offset).c_str());

        goto err;
    }
    dmsg("");
    return arr;
err:
    dmsg("err");
    if (arr)
        delete arr;
    return nullptr;
}

json *load_str_obj(const std::string &str, size_t &offset)
{
    if ('{' != str.at(offset))
        return nullptr;
    // skip {
    ++offset;
    dmsg("str:%s", str.c_str());

    json *obj = new(std::nothrow) json(json::type_t::OBJ);//json("", new json(""));//json::type_t::OBJ);
    if (!obj)
        return nullptr;

    dmsg("");

    for (; offset < str.length();) {
        if ('}' == str.at(offset)) 
            break;
        dmsg("");
        dmsg("str:%s", str.c_str());
        json *iter = nullptr;
        if ('"' == str.at(offset)) {
            dmsg("");
            iter = load_str_key_val(str, offset);
            if (!iter) 
                goto err;
            dmsg("");
        }
        dmsg("");
        if (!iter)
            goto err;
        dmsg("insert iter:%s, type:%d", iter->to_str().c_str(), iter->type());
        json::obj_t *o = obj->obj();
        if (!o)
            goto err;
        assert(iter);
        assert(o);
        o->push_back(iter);
        if (',' == str.at(offset)) {
            ++offset;
            continue;
        }
        dmsg("");
    }
    dmsg("");
    if ('}' != str.at(offset)) {
        goto err;
    }
    // skip }
    ++offset;
    dmsg("");

    return obj;
err:
    if (obj)
        delete obj;
    return nullptr;
}

json *json::load_str(const str_t &str)
{
    str_t parser;
    bool left = false;
    bool right = false;

    dmsg("");

    for (const auto &ch: str) {
        if (left && '"' == ch)
            right = true;
        if (!right && '"' == ch)
            left = true;

        if (left || (!left && ' ' != ch))
            parser += ch;

        if (left && right) {
            left = false;
            right = false;
        }
    }
    if (parser.empty())
        return nullptr;

    dmsg("parser: %s", parser.c_str());

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

    std::stringstream instream;
    instream << infile.rdbuf();
    str_t instr(instream.str());


    dmsg("instr: %s\n", instr.c_str());

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

    json jarr_str("", { "1" , "2", "3"});
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
        << jarr_str.to_str() << "\n"
        << jarr_num.to_str() << "\n"
        << root.to_str() << "\n"
        << root["B"]->to_str() << "\n"
        << "\n";

    cout << "out > 1.js" << "\n";
    root.to_file("1.js");

    if (input) {
        cout << "read input file name: " << input << "\n";

        json *js = json::load_file(input);
        if (!js)
            return -1;

        cout << "read " << input << ": " << "\n" 
            << js->to_str() << "\n";
        delete js;
    }

    return 0;
}

#endif
