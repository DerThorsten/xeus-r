/***************************************************************************
* Copyright (c) 2022,                                          
*                                                                          
* Distributed under the terms of the GNU General Public License v3.                 
*                                                                          
* The full license is in the file LICENSE, distributed with this software. 
****************************************************************************/

#include <Rcpp.h>
// [[Rcpp::plugins(cpp14)]]


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
  

    nl::json interpreter::execute_request_impl(int execution_counter, // Typically the cell number
                                               const std::string& code, // Code to execute
                                               bool /*silent*/,
                                               bool /*store_history*/,
                                               nl::json /*user_expressions*/,
                                               bool /*allow_stdin*/)
    {
        nl::json kernel_res;

        SEXP ret;
        m_R.parseEval(code, ret);

        nl::json pub_data;
        // pub_data["text/plain"] = code;
        publish_execution_result(execution_counter, std::move(pub_data), nl::json());

        kernel_res["status"] = "ok";
        kernel_res["payload"] = nl::json::array();
        kernel_res["user_expressions"] = nl::json::object();

        return kernel_res;

    }

}
