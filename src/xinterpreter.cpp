/***************************************************************************
* Copyright (c) 2022,                                          
*                                                                          
* Distributed under the terms of the GNU General Public License v3.                 
*                                                                          
* The full license is in the file LICENSE, distributed with this software. 
****************************************************************************/

#include <string>
#include <vector>
#include <iostream>

#include "nlohmann/json.hpp"

#include "xeus/xinput.hpp"
#include "xeus/xinterpreter.hpp"

#include "xeus-r/xinterpreter.hpp"

namespace nl = nlohmann;

namespace xr
{


    interpreter::interpreter() : m_R()
    {
        xeus::register_interpreter(this);
    }

   
    void interpreter::configure_impl()
    {
        register_io();
    }

    nl::json interpreter::is_complete_request_impl(const std::string& code)
    {
        nl::json result;
        result["status"] = "complete";
        if (code.compare("incomplete") == 0)
        {
            result["status"] = "incomplete";
            result["indent"] = "   ";
        }
        else if(code.compare("invalid") == 0)
        {
            result["status"] = "invalid";
            result["indent"] = "   ";
        }
        return result;
    }
    nl::json interpreter::complete_request_impl(const std::string&  code,
                                                     int cursor_pos)
    {
        nl::json result;

        // Code starts with 'H', it could be the following completion
        if (code[0] == 'H')
        {
            result["status"] = "ok";
            result["matches"] = {
                std::string("Hello"), 
                std::string("Hey"), 
                std::string("Howdy")
            };
            result["cursor_start"] = 5;
            result["cursor_end"] = cursor_pos;
        }
        // No completion result
        else
        {
            result["status"] = "ok";
            result["matches"] = nl::json::array();
            result["cursor_start"] = cursor_pos;
            result["cursor_end"] = cursor_pos;
        }

        return result;
    }

    nl::json interpreter::inspect_request_impl(const std::string& /*code*/,
                                                      int /*cursor_pos*/,
                                                      int /*detail_level*/)
    {
        nl::json result;
        result["status"] = "ok";
        result["found"] = true;
        
        result["data"] = {{std::string("text/plain"), std::string("hello!")}};
        result["metadata"] = {{std::string("text/plain"), std::string("hello!")}};
         
        return result;
    }

   
    void interpreter::shutdown_request_impl() {
        std::cout << "Bye!!" << std::endl;
    }


    nl::json interpreter::kernel_info_request_impl()
    {
        nl::json result;
        result["implementation"] = "xr";
        result["implementation_version"] = XEUS_R_VERSION;
        result["banner"] = "xr";
        result["language_info"]["name"] = "R";
        result["language_info"]["version"] = "3.6";
        result["language_info"]["mimetype"] = "text/x-rsrc";
        result["language_info"]["file_extension"] = "R";
        return result;
    }

}
