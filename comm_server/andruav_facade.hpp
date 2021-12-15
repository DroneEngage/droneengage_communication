#ifndef ANDRUAV_FACADE_H_
#define ANDRUAV_FACADE_H_

namespace uavos
{

namespace andruav_servers 
{
    enum ENUM_TASK_SCOPE
    {
        SCOPE_GLOBAL    =0,
        SCOPE_ACCOUNT   =1,
        SCOPE_GROUP     =2,
        SCOPE_PARTY_ID  =3
    };

    /**
     * @brief High level APIs sent to Andruav Communication Server
     * 
     */
    class CAndruavFacade
    {
        public:
            //https://stackoverflow.com/questions/1008019/c-singleton-design-pattern
            
            static CAndruavFacade& getInstance()
            {
                static CAndruavFacade instance;

                return instance;
            };

        public:
            CAndruavFacade(CAndruavFacade const&) = delete;
            void operator=(CAndruavFacade const&) = delete;

        private:

            CAndruavFacade() 
            {
            };
    
        public:
            
            ~CAndruavFacade (){};
            
        
        public:
            void API_sendID (const std::string& target_party_id) const ;
            void API_requestID (const std::string& target_party_id) const ;
            void API_sendCameraList(const bool reply, const std::string& target_party_id) const ;
            void API_sendErrorMessage (const std::string& target_party_id, const int& error_number, const int& info_type, const int& notification_type, const std::string& description) const ;
     
            void API_loadTasksByScope(const ENUM_TASK_SCOPE scope, const int task_type) const;
            void API_loadTasksByScopeGlobal(const int task_type) const;
            void API_loadTasksByScopeAccount(const int task_type) const;
            void API_loadTasksByScopeGroup(const int task_type) const;
            void API_loadTasksByScopePartyID(const int task_type) const;

            void API_loadTask(const int larger_than_SID, const std::string& account_id, const std::string& party_sid, const std::string& group_name, const std::string& sender, const std::string& receiver, const int msg_type, bool is_permanent ) const;
    };

}

}

#endif
