#include <cctype>
#include <new>
#include <sstream>
#include <fstream>
#include <cstring>
#include <cassert>
#include <cstdio>


#include <map>
#include <vector>
#include <string>
#include <initializer_list>

#include "json.h"

class json {
    public:
        typedef enum { BOL, NUM, STR, ARR, OBJ, NUL, } type_t;
        typedef bool bol_t;
        typedef double num_t;
        typedef std::string str_t;
        typedef std::vector<json *> arr_t;
        typedef std::map<str_t, json *> obj_t;

    public:
        json(const type_t &type);
        json(const str_t &key);
        json(const str_t &key, const bol_t       &bol);
        json(const str_t &key, const int         &num);
        json(const str_t &key, const num_t       &num);
        json(const str_t &key, const str_t       &str);
        json(const str_t &key, const std::initializer_list<int>    &li);
        json(const str_t &key, const std::initializer_list<num_t>  &li);
        json(const str_t &key, const std::initializer_list<str_t>  &li);
        json(const str_t &key, const std::initializer_list<json *> &li);
        ~json();

        json *operator=(const bol_t &bol);
        json *operator=(const int &num);
        json *operator=(const num_t &num);
        json *operator=(const char *str);
        json *operator=(const str_t &str);
        json *operator=(const arr_t &arr);
        json *operator=(const obj_t &obj);
        json *operator=(const json &js);
        json *operator[](const str_t &key);

        int set_type(const type_t &type);
        const type_t &type();
        void set_key(const str_t &key);
        const str_t &key();
        int set_bol(const bol_t &bol);
        bol_t bol();
        int set_num(const num_t &num);
        num_t num();
        str_t *str();
        arr_t *arr();
        obj_t *obj();
        const str_t to_str();
        int to_file(const char *file_name);
        static json *load_str(const str_t &str);
        static json *load_file(const char *file_name);

    private:
        bool is_num(const str_t &str);
        bool is_bol(const str_t &str);
        void del_type();
        void del_str();
        void del_arr();
        void del_obj();
        static json *load_str_arr(const str_t &str, size_t &offset);
        static json *load_str_obj(const str_t &str, size_t &offset);
        static json *load_str_key_val(const str_t &str, size_t &offset);
        static json *dup_json(const json *js);
        static arr_t *dup_arr(const arr_t *arr);
        static obj_t *dup_obj(const obj_t *obj);

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



#define dmsg(fmt, ...) if (g_enable_json_dbg) fprintf(stdout, "[%s:%s:%d] " fmt "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__)
static bool g_enable_json_dbg = false;

json::json(const type_t &type):
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

json::json(const str_t &key): 
    m_type(NUL), m_key(key) 
{}

json::json(const str_t &key, const bol_t     &bol): 
    m_type(BOL), m_key(key), m_bol(bol) 
{}

json::json(const str_t &key, const int       &num): 
    m_type(NUM), m_key(key), m_num(num) 
{}

json::json(const str_t &key, const num_t     &num): 
    m_type(NUM), m_key(key), m_num(num) 
{}

json::json(const str_t &key, const str_t     &str): 
    m_type(STR), m_key(key), m_str(new(std::nothrow) str_t(str)) 
{}

json::json(const str_t &key, const std::initializer_list<int>    &li):
    m_type(ARR), m_key(key), m_arr(new(std::nothrow) arr_t)
{ 
    for (const auto &num: li) m_arr->push_back(new(std::nothrow) json("", num));
}

json::json(const str_t &key, const std::initializer_list<num_t>  &li):
    m_type(ARR), m_key(key), m_arr(new(std::nothrow) arr_t)
{ 
    for (const auto &num: li) m_arr->push_back(new(std::nothrow) json("", num)); 
}

json::json(const str_t &key, const std::initializer_list<str_t>  &li):
    m_type(ARR), m_key(key), m_arr(new(std::nothrow) arr_t)
{ 
    for (const auto &str: li) m_arr->push_back(new(std::nothrow) json("", str)); 
}

json::json(const str_t &key, const std::initializer_list<json *> &li):
    m_type(OBJ), m_key(key), m_obj(new(std::nothrow) obj_t)
{
    for (const auto &obj: li) (*m_obj)[obj->m_key] = obj; 
}

json::~json()
{
    switch (m_type) {
        case STR: del_str(); break;
        case ARR: del_arr(); break;
        case OBJ: del_obj(); break;
        default: break;
    }
}

json *json::operator=(const bol_t &bol)
{
    if (set_type(BOL)) {
        dmsg("fail to set type bol");
        return nullptr;
    }

    m_bol = bol;

    return this;
}

json *json::operator=(const int &num)
{
    return operator=(num_t(num));
}

json *json::operator=(const num_t &num)
{
    if (set_type(NUM)) {
        dmsg("fail to set type num");
        return nullptr;
    }

    m_num = num;

    return this;
}

json *json::operator=(const char *str)
{
    return operator=(str_t(str));
}

json *json::operator=(const str_t &str)
{
    if (set_type(STR)) {
        dmsg("fail to set type str");
        return nullptr;
    }

    *m_str = str;

    return this;
}

json *json::operator=(const arr_t &arr)
{
    arr_t *new_arr = new(std::nothrow) arr_t;
    if (!new_arr) {
        dmsg("fail to new arr_t");
        goto err;
    }

    for (const auto &iter: arr) {
        json *dup_js = dup_json(iter);
        if (!dup_js) {
            dmsg("fail to dup arr");
            goto err;
        }

        new_arr->push_back(dup_js);
    }

    del_type();
    m_type = ARR;
    m_arr = new_arr;

    return this;

err:
    if (new_arr)
        delete new_arr;
    return nullptr;
}

json *json::operator=(const obj_t &obj)
{
    obj_t *new_obj = new(std::nothrow) obj_t;
    if (!new_obj)
        goto err;

    for (const auto &iter: obj) {
        json *dup_js = dup_json(iter.second);
        if (!dup_js) {
            dmsg("fail to dup obj key:%s", iter.first.c_str());
            goto err;
        }

        (*new_obj)[iter.first] = dup_js;
    }
    del_type();
    m_type = OBJ;
    m_obj = new_obj;

    return this;

err:
    if (new_obj)
        delete new_obj;
    return nullptr;
}

json *json::operator=(const json &js)
{
    str_t *new_str = nullptr;
    arr_t *new_arr = nullptr;
    obj_t *new_obj = nullptr;

    switch (js.m_type) {
        case STR:
            new_str = new(std::nothrow) str_t(*js.m_str);
            if (!new_str) {
                dmsg("fail to dup str");
                goto err;
            }
            break;
        case ARR:
            new_arr = dup_arr(js.m_arr);
            if (!new_arr) {
                dmsg("fail to dup arr");
                goto err;
            }
            break;
        case OBJ:
            new_obj = dup_obj(js.m_obj);
            if (!new_obj) {
                dmsg("fail to dup obj");
                goto err;
            }
            break;
        case BOL: 
        case NUM: 
        case NUL:
        default:
            break;
    }

    del_type();

    switch (js.m_type) {
        case BOL: m_bol = js.m_bol; break;
        case NUM: m_num = js.m_num; break;
        case STR: m_str = new_str; break;
        case ARR: m_arr = new_arr; break;
        case OBJ: m_obj = new_obj; break;
        case NUL: break;
        default: break;
    }

    m_type = js.m_type;

    return this;

err:
    if (new_str)
        delete new_str;
    if (new_obj)
        delete new_obj;
    if (new_arr)
        delete new_arr;

    return nullptr;
}

json *json::operator[](const str_t &key)
{ 
    if (OBJ != m_type) {
        dmsg("not obj");
        return nullptr;
    }

    auto iter = m_obj->find(key);
    if (iter != m_obj->end())
        return iter->second;

    json *add = new(std::nothrow) json(key);
    if (!add) {
        dmsg("fail to add key: %s", key.c_str());
        return nullptr;
    }

    (*m_obj)[key] = add;
    return add;
}

int json::set_type(const type_t &type)
{
    if (type == m_type)
        return 0;

    str_t *new_str = nullptr;
    arr_t *new_arr = nullptr;
    obj_t *new_obj = nullptr;

    switch (type) {
        case STR:
            new_str = new(std::nothrow) str_t();
            if (!new_str)
                goto err;
            break;
        case ARR:
            new_arr = new(std::nothrow) arr_t();
            if (!new_arr)
                goto err;
            break;
        case OBJ:
            new_obj = new(std::nothrow) obj_t();
            if (!new_obj)
                goto err;
            break;
        case BOL:
        case NUM:
        case NUL:
        default: 
            break;
    }

    del_type();

    switch (type) {
        case BOL: m_bol = false; break;
        case NUM: m_num = 0; break;
        case STR: m_str = new_str; break;
        case ARR: m_arr = new_arr; break;
        case OBJ: m_obj = new_obj; break;
        case NUL: break;
        default: break;
    }

    m_type = type;

    return 0;
err:
    if (new_str)
        delete new_str;
    if (new_arr)
        delete new_arr;
    if (new_obj)
        delete new_obj;

    return -1;
}

const json::type_t &json::type()
{
    return m_type; 
}

void json::set_key(const str_t &key)
{
    m_key = key;
}

const json::str_t &json::key()
{
    return m_key;
}

int json::set_bol(const bol_t &bol)
{
    if (BOL != m_type)
        return -1;

    m_bol = bol;
    return 0;
}

json::bol_t json::bol()
{
    return (BOL == m_type) ? m_bol : false;
}

int json::set_num(const num_t &num)
{
    if (NUM != m_type)
        return -1;

    m_num = num;
    return 0;
}

json::num_t json::num()
{
    return (NUM == m_type) ? m_num : 0;
}

json::str_t *json::str()
{
    return (STR == m_type) ? m_str : nullptr;
}

json::arr_t *json::arr()
{
    return (ARR == m_type) ? m_arr : nullptr;
}

json::obj_t *json::obj()
{
    return (OBJ == m_type) ? m_obj : nullptr;
}

bool json::is_num(const str_t &str) 
{
    for (const auto &ch: str)
        if (!std::isdigit(ch) && '.' != ch)
            return false;
    return true;
}

bool json::is_bol(const str_t &str) 
{
    return (((str.length() == (sizeof("true")-1)) && 
                !strcasecmp("true", str.c_str())) ||
            ((str.length() == (sizeof("false")-1)) &&
                !strcasecmp("false", str.c_str())));
}

void json::del_type()
{
    switch (m_type) {
        case STR: del_str(); break;
        case ARR: del_arr(); break;
        case OBJ: del_obj(); break;
        case BOL: m_bol = false; break;
        case NUM: m_num = 0; break;
        case NUL:
        default: break;
    }
    m_type = NUL;
}

void json::del_str()
{
    assert(STR == m_type);
    delete m_str;
    m_str = nullptr;
    m_type = NUL;
}

void json::del_arr()
{
    assert(ARR == m_type);
    for (auto &iter: *m_arr) {
        if (!iter) {
            continue;
        }

        delete iter;
    }
    m_arr->clear();
    delete m_arr;
    m_arr = nullptr;
    m_type = NUL;
}

void json::del_obj()
{
    assert(OBJ == m_type);
    for (auto &obj: *m_obj) {
        if (!obj.second) {
            continue;
        }

        delete obj.second;
    }
    m_obj->clear();
    delete m_obj;
    m_obj = nullptr;
    m_type = NUL;
}

const json::str_t json::to_str()
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
            out_str += '"' + *str() + '"';
            break;
        case BOL:
            out_str += (bol() ? "true" : "false");
            break;
        case ARR:
            out_str += "[ ";
            if (arr()->empty()) {
                out_str += " ]";
                break;
            }
            for (const auto &iter: *arr()) {
                if (!iter)
                    continue;
                if (!is_first)
                    out_str += ", ";
                else
                    is_first = false;
                out_str += iter->to_str();
            }
            out_str += " ]";
            break;
        case OBJ:
            dmsg("");
            out_str += "{ ";
            if (obj()->empty()) {
                out_str += " }";
                break;
            }
            for (const auto &iter: *obj()) {
                if (!iter.second)
                    continue;
                if (!is_first)
                    out_str += ", ";
                else
                    is_first = false;

                out_str += iter.second->to_str();
            }
            out_str += " }";
            break;
        default:
            break;
    }

    return out_str;
}

json *json::load_str_key_val(const str_t &str, size_t &offset)
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

json *json::load_str_arr(const str_t &str, size_t &offset)
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

json *json::load_str_obj(const str_t &str, size_t &offset)
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
        assert(o);
        assert(iter);
        (*o)[iter->key()] = iter;
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

    std::ofstream outfile(file_name, std::ios::out | std::ios::trunc);

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

    infile.close();

    dmsg("instr: %s\n", instr.c_str());

    return json::load_str(instr);
}

json *json::dup_json(const json *js)
{
    assert(js);

    switch (js->m_type) {
        case NUM: return new(std::nothrow) json(js->m_key, js->m_num);
        case BOL: return new(std::nothrow) json(js->m_key, js->m_bol);
        case STR: return new(std::nothrow) json(js->m_key, js->m_str);
        case ARR: return new(std::nothrow) json(js->m_key, dup_arr(js->m_arr));
        case OBJ: return new(std::nothrow) json(js->m_key, dup_obj(js->m_obj));
        case NUL:
        default:  return new(std::nothrow) json(js->m_key);
    }
}

json::arr_t *json::dup_arr(const arr_t *arr)
{
    assert(arr);

    arr_t *new_arr = new(std::nothrow) arr_t;
    if (!new_arr)
        return nullptr;

    for (const auto &iter: *arr) {
        json *dup_iter = dup_json(iter);
        if (!dup_iter)
            goto err;

        new_arr->push_back(dup_iter);
    }

    return new_arr;

err:
    if (new_arr)
        delete new_arr;
    return nullptr;
}

json::obj_t *json::dup_obj(const obj_t *obj)
{
    assert(obj);

    obj_t *new_obj = new(std::nothrow) obj_t;
    if (!new_obj)
        return nullptr;

    for (const auto &iter: *obj) {
        json *dup_iter = dup_json(iter.second);
        if (!dup_iter)
            goto err;

        (*new_obj)[dup_iter->m_key] = dup_iter;
    }

    return new_obj;

err:
    if (new_obj)
        delete new_obj;
    return nullptr;
}

#ifdef __XTEST__

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

    cout << "before: " << jobj.to_str() << "\n";
    
    jobj["fuck"][0] = true;
    jobj["moe"][0] = "?";
    jobj["*"][0] = 996.007;
    jobj["-"][0] = 0;
    jobj["num"][0] = jobj["*"]->num() + 1000;
    jobj["func"][0] = json(json::OBJ);
    jobj["func"][0]["add"][0] = jobj["num"]->num() + 1;

    cout << "after:  " << jobj.to_str() << "\n";

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

#endif//__XTEST__
