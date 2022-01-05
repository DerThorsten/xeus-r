/***************************************************************************
* Copyright (c) 2022,                                          
*                                                                          
* Distributed under the terms of the GNU General Public License v3.                 
*                                                                          
* The full license is in the file LICENSE, distributed with this software. 
****************************************************************************/

#include <RInside.h>
#include "xeus/xinterpreter.hpp"
#include "xeus-r/xinterpreter.hpp"
namespace xr
{

    void xeus_print(const std::string & to_print)
    {
        xeus::get_interpreter().publish_stream("stdout", to_print);
    }

    void interpreter::register_io()
    {
        m_R["xeus_print"] = Rcpp::InternalFunction( &xeus_print ) ;
    }
}
