/*
*OLC
*Revised and rewritten.
*/
#pragma once
#include <sstream>
#include <string>
#include <unordered_map>
#include <functional>
#include <utility>
#include "conf.h"
#include "mud.h"
#include "baseObject.h"
#include "socket.h"
#include "player.h"
#include "command.h"
#include "utils.h"

class OlcGroup;
//olcset command subcommands
#define COS_OBJECT 1
#define COS_ROOM 2
#define COS_MOB 3

enum class OLCGROUP
{
    NONE,
    STATIC,
    ROOM,
    NPC,
    COMPONENT
};
enum class OLCDT
{
    INTEGER,
    STRING,
    DECIMAL,
    BOOLEAN,
    FLG,
    ENUM,
    EDITOR
};
#define OF_NORMAL 1<<1
#define OF_WIZ 1<<2

class IOlcEntry
{
protected:
    std::string _name;
    std::string _help;
    FLAG _flag;
    OLCDT _dt;
public:
    IOlcEntry(const std::string &name, const std::string &help, FLAG flag, OLCDT dt);
    virtual ~IOlcEntry();
    std::string GetName() const;
    void SetName(const std::string& v);
    std::string GetHelp() const;
    void SetHelp(const std::string& v);
    FLAG GetFlag() const;
    void SetFlag(FLAG v);
    OLCDT GetInputType() const;
    void SetInputType(OLCDT v);
    virtual bool HandleInput(Player* mobile, void* o, const std::string& input) = 0;
    virtual bool RequiresInput()
    {
        return true;
    }
};

template <class C>
class OlcStringEntry:public IOlcEntry
{
    typedef std::function<std::string (const C*)> StringGetter;
    typedef std::function<void (C*, const std::string&)> StringSetter;
protected:
    StringGetter _getter;
    StringSetter _setter;
public:
    OlcStringEntry(const std::string &name, const std::string &help, FLAG flag, OLCDT dt, const StringGetter getter, const StringSetter setter)
        :IOlcEntry(name, help, flag, dt)
    {
        _setter = setter;
        _getter = getter;
    }
    bool HandleInput(Player* mobile, void* o, const std::string& input)
    {
        _setter(static_cast<C*>(o), input);
        return true;
    }
};

template <class C>
class OlcEditorEntry:public IOlcEntry
{
    typedef std::function<std::string (const C*)> StringGetter;
    typedef std::function<void (C*, const std::string&)> StringSetter;
protected:
    StringGetter _getter;
    StringSetter _setter;
public:
    OlcEditorEntry(const std::string &name, const std::string &help, FLAG flag, OLCDT dt, const StringGetter getter, const StringSetter setter)
        :IOlcEntry(name, help, flag, dt)
    {
        _setter = setter;
        _getter = getter;
    }
    EVENT(Save)
    {
        Editor* ed = static_cast<EditorSavedArgs*>(args)->editor;
        C* o = static_cast<C*>(ed->GetArg());
        std::vector<std::string>* lines = ed->GetLines();
        std::stringstream st;

        for(std::string line: *lines)
            {
                st << std::endl;
                st << line;
            }

        _setter(o, st.str());
    }

    bool HandleInput(Player* mobile, void* o, const std::string& input)
    {
        std::vector<std::string> lines;
        Editor* editor = new Editor();
        editor->SetArg(o);
        Tokenize(_getter(static_cast<C*>(o)), lines, "\n");
        editor->events.AddCallback("save", std::bind(&OlcEditorEntry<C>::Save, this, std::placeholders::_1, std::placeholders::_2));
        editor->EnterEditor(mobile);
        for(std::string line: lines)
            {
                editor->Add(line, true);
            }
        return true;
    }
    bool RequiresInput()
    {
        return false;
    }
};

template <class C, typename E>
class OlcEnumEntry:public IOlcEntry
{
    typedef std::function<E (const C*)> EnumGetter;
    typedef std::function<void (C*, E)> EnumSetter;
    std::map<std::string, E> _vals;
protected:
    EnumGetter _getter;
    EnumSetter _setter;
public:
    OlcEnumEntry(const std::string &name, const std::string &help, FLAG flag, OLCDT dt, const EnumGetter getter, const EnumSetter setter, const std::map<std::string, E> &vals)
        :IOlcEntry(name, help, flag, dt)
    {
        _setter = setter;
        _getter = getter;
        _vals = vals;
    }
    bool HandleInput(Player* mobile, void* o, const std::string& input)
    {
        int cols = 0;

        if (input == "list")
            {
                std::vector<std::string> vals;
                for (auto it: _vals)
                    {
                        vals.push_back(it.first);
                    }

                if (vals.size() / 4 > 2)
                    {
                        cols = 4;
                    }
                else
                    {
                        cols = (vals.size() > 1? 2 : 1);
                    }

                mobile->Message(MSG_LIST, Columnize(&vals, cols));
                return true;
            }

        for (auto it:_vals)
            {
                if (input == it.first)
                    {
                        _setter(static_cast<C*>(o), it.second);
                        return true;
                    }
            }

        return false;
    }
};

//converts the olc  data type to a string representation.
std::string OlcTypeToString(OLCDT dt);

//we use the same command for oset, mset and rset.
//we use subcmd to tell us what command was actually called.
class CMDOlcSet:public Command
{
public:
    CMDOlcSet();
    void ShowGroup(Player* mobile, OlcGroup* group);
    BOOL Execute(const std::string &verb, Player* mobile,std::vector<std::string> &args,int subcmd);
};

BOOL InitializeOlc();
