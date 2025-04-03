
#include "../helpers/colors.hpp"
#include "../helpers/helpers.hpp"
#include "../global.hpp"
#include "../messages.hpp"
#include "mission_manager_base.hpp"
#include "../de_broker/de_modules_manager.hpp"
#include "../comm_server/andruav_parser.hpp"
#include "../comm_server/andruav_comm_server.hpp"
#include "./mission_file.hpp"

using namespace de::mission;


void CMissionManagerBase::clearModuleMissionItems ()
{
    m_last_executed_mission_id = "";
    m_module_missions.clear();
    m_module_missions_by_de_events.clear();
    CMissionFile::getInstance().deleteMissionFile("de_plan.json");
}


/**
 * @brief Read plan file as JSON object and extract DE mission items from it.
 * You need to read DE-Mission and link them with Mavlink Mission Item if exists.
 * @param plan Plan as JSON object.
 */
void CMissionManagerBase::extractPlanModule (const Json_de& plan)
{

try
    {
        clearModuleMissionItems();
        
        if (std::string(plan["fileType"]).find("de_plan") != std::string::npos)
        {
            CMissionFile::getInstance().writeMissionFile("de_plan.json", plan.dump());
            
            if (plan.contains("de_mission"))
            {
                // there is a waypoint data
                const Json_de de_mission = plan["de_mission"];

                Json_de modules;
                if (validateField(de_mission, "modules",Json_de::value_t::array))
                {
                    const Json_de modules = de_mission["modules"];
                    
                    for (const auto& module_mission_item : modules) {
                        
                        std::cout << "module_mission_item:" << module_mission_item << std::endl; 
                        
                        Json_de module_mission_item_commands = module_mission_item["c"];
                        
                        std::string module_linked = "";
                        
                        
                        const bool ls_avail = validateField(module_mission_item, "ls", Json_de::value_t::string);
                        
                        if (ls_avail)
                        {
                            module_linked = module_mission_item["ls"];
                        }

                        for (auto& module_mission_item_single_command : module_mission_item_commands) {

                            /// module_mission_item_single_command[LINKED_TO_STEP] = module_linked;
                            module_mission_item_single_command[INTERMODULE_ROUTING_TYPE] = CMD_TYPE_INTERMODULE; 
                            // check if I am waiting for event.
                            if (validateField(module_mission_item,WAITING_EVENT,Json_de::value_t::string))
                            {
                                std::string de_event_id = module_mission_item[WAITING_EVENT].get<std::string>();
                                addModuleMissionItemByEvent (de_event_id, module_mission_item_single_command);
                            }
                            std::cout << "module_mission_item_single_command:" << module_mission_item_single_command.dump() << std::endl; 

                            if (ls_avail)
                            {
                                addModuleMissionItem(module_linked, module_mission_item_single_command); 
                            }
                        }   
                    }


                    std::cout << "m_module_missions:" << m_module_missions.size() << "  m_module_missions_by_de_events:" << m_module_missions_by_de_events.size() << std::endl;
                }
            }
        }
        
    }
    catch(const std::exception& e)
    {
        std::cerr << "ss" << e.what() << '\n';
    }

}

void CMissionManagerBase::fireWaitingCommands (const std::string de_event_sid)
{
    // get list of commands std::vector that are attached to this event if exists
    if (m_module_missions_by_de_events.find(de_event_sid) != m_module_missions_by_de_events.end()) 
    {
        std::vector<Json_de> cmds = m_module_missions_by_de_events[de_event_sid];
        
        // execute each command by sending it to comm-parser and other modules.
        for (const auto cmd : cmds)
        {
            const int command_type = cmd[ANDRUAV_PROTOCOL_MESSAGE_TYPE].get<int>();
            
            
            const std::string sender = de::CAndruavUnitMe::getInstance().getUnitInfo().party_id;
            
            
            #ifdef DEBUG
            std::cout << _LOG_CONSOLE_TEXT << "fireWaitingCommands:" << cmd.dump() << _NORMAL_CONSOLE_TEXT_ << std::endl;
            #endif

            const std::string cmd_text = cmd.dump();

            
            // here we process the internal commands waiting for the fired event.
            // not the fired event itself.
            // the fired event is forwarded to module in the andruav_parser... not here.
            switch (command_type)
            {
                case TYPE_AndruavMessage_RemoteExecute:
                {
                    de::andruav_servers::CAndruavParser::getInstance().parseRemoteExecuteCommand(sender, cmd);
                }
                break;

                default:
                {
                    de::andruav_servers::CAndruavParser::getInstance().parseCommand(sender, command_type, cmd);
                }
                break;
            }
            
            // here we forward the internal commands waiting for the fired event.
            // not the fired event itself.
            // the fired event is forwarded to module in the andruav_parser... not here.
            de::comm::CUavosModulesManager::getInstance().processIncommingServerMessage(sender, command_type, cmd_text.c_str(), cmd_text.length(), std::string());
        }
    }

    return ; 
}

void CMissionManagerBase::getCommandsAttachedToMavlinkMission(const std::string mission_i)
{
    
    if (mission_i == m_last_executed_mission_id) return ;
    
    m_last_executed_mission_id = ""; // reset so that if we are back to previous mission attached module missions will be re-triggered. 
    // get list of commands std::vector that are attached to this event if exists
    if (m_module_missions.find(mission_i) != m_module_missions.end()) 
    {
        m_last_executed_mission_id = mission_i;
        std::vector<Json_de> cmds = m_module_missions[mission_i];
        
        // execute each command by sending it to comm-parser and other modules.
        for (const auto cmd : cmds)
        {
            const int command_type = cmd[ANDRUAV_PROTOCOL_MESSAGE_TYPE].get<int>();
            
            
            const std::string sender = de::CAndruavUnitMe::getInstance().getUnitInfo().party_id;
            
            
            #ifdef DEBUG
            std::cout << _LOG_CONSOLE_TEXT << "fireWaitingCommands:" << cmd.dump() << _NORMAL_CONSOLE_TEXT_ << std::endl;
            #endif

            const std::string cmd_text = cmd.dump();

            
            
            
            // here we process the internal commands waiting for the fired event.
            // not the fired event itself.
            // the fired event is forwarded to module in the andruav_parser... not here.
            switch (command_type)
            {
                case TYPE_AndruavMessage_RemoteExecute:
                {
                    de::andruav_servers::CAndruavParser::getInstance().parseRemoteExecuteCommand(sender, cmd);
                }
                break;

                default:
                {
                    de::andruav_servers::CAndruavParser::getInstance().parseCommand(sender, command_type, cmd);
                }
                break;
            }
            
            
            // here we forward the internal commands waiting for the fired event.
            // not the fired event itself.
            // the fired event is forwarded to module in the andruav_parser... not here.
            de::comm::CUavosModulesManager::getInstance().processIncommingServerMessage(sender, command_type, cmd_text.c_str(), cmd_text.length(), std::string());
        }
    }

    return ; 
}





