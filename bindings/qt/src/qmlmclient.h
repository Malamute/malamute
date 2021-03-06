/*
################################################################################
#  THIS FILE IS 100% GENERATED BY ZPROJECT; DO NOT EDIT EXCEPT EXPERIMENTALLY  #
#  Read the zproject/README.md for information about making permanent changes. #
################################################################################
*/
#ifndef Q_MLM_CLIENT_H
#define Q_MLM_CLIENT_H

#include "qmalamute.h"

class QT_MLM_EXPORT QMlmClient : public QObject
{
    Q_OBJECT
public:

    //  Copy-construct to return the proper wrapped c types
    QMlmClient (mlm_client_t *self, QObject *qObjParent = 0);

    //  Create a new mlm_client, return the reference if successful,   
    //  or NULL if construction failed due to lack of available memory.
    explicit QMlmClient (QObject *qObjParent = 0);

    //  Destroy the mlm_client and free all memory used by the object.
    ~QMlmClient ();

    //  Return actor, when caller wants to work with multiple actors and/or
    //  input sockets asynchronously.                                      
    QZactor * actor ();

    //  Return message pipe for asynchronous message I/O. In the high-volume case,
    //  we send methods and get replies to the actor, in a synchronous manner, and
    //  we send/recv high volume message data to a second pipe, the msgpipe. In   
    //  the low-volume case we can do everything over the actor pipe, if traffic  
    //  is never ambiguous.                                                       
    QZsock * msgpipe ();

    //  Return true if client is currently connected, else false. Note that the   
    //  client will automatically re-connect if the server dies and restarts after
    //  a successful first connection.                                            
    bool connected ();

    //  Set PLAIN authentication username and password. If you do not call this, the    
    //  client will use NULL authentication. TODO: add "set curve auth".                
    //  Returns >= 0 if successful, -1 if interrupted.                                  
    int setPlainAuth (const QString &username, const QString &password);

    //  Connect to server endpoint, with specified timeout in msecs (zero means wait    
    //  forever). Constructor succeeds if connection is successful. The caller may      
    //  specify its address.                                                            
    //  Returns >= 0 if successful, -1 if interrupted.                                  
    int connect (const QString &endpoint, quint32 timeout, const QString &address);

    //  Prepare to publish to a specified stream. After this, all messages are sent to  
    //  this stream exclusively.                                                        
    //  Returns >= 0 if successful, -1 if interrupted.                                  
    int setProducer (const QString &stream);

    //  Consume messages with matching subjects. The pattern is a regular expression    
    //  using the CZMQ zrex syntax. The most useful elements are: ^ and $ to match the  
    //  start and end, . to match any character, \s and \S to match whitespace and      
    //  non-whitespace, \d and \D to match a digit and non-digit, \a and \A to match    
    //  alphabetic and non-alphabetic, \w and \W to match alphanumeric and              
    //  non-alphanumeric, + for one or more repetitions, * for zero or more repetitions,
    //  and ( ) to create groups. Returns 0 if subscription was successful, else -1.    
    //  Returns >= 0 if successful, -1 if interrupted.                                  
    int setConsumer (const QString &stream, const QString &pattern);

    //  Offer a particular named service, where the pattern matches request subjects    
    //  using the CZMQ zrex syntax.                                                     
    //  Returns >= 0 if successful, -1 if interrupted.                                  
    int setWorker (const QString &address, const QString &pattern);

    //  Send STREAM SEND message to server, takes ownership of message
    //  and destroys message when done sending it.                    
    int send (const QString &subject, QZmsg *content);

    //  Send MAILBOX SEND message to server, takes ownership of message
    //  and destroys message when done sending it.                     
    int sendto (const QString &address, const QString &subject, const QString &tracker, quint32 timeout, QZmsg *content);

    //  Send SERVICE SEND message to server, takes ownership of message
    //  and destroys message when done sending it.                     
    int sendfor (const QString &address, const QString &subject, const QString &tracker, quint32 timeout, QZmsg *content);

    //  Receive message from server; caller destroys message when done
    QZmsg * recv ();

    //  Return last received command. Can be one of these values:
    //      "STREAM DELIVER"                                     
    //      "MAILBOX DELIVER"                                    
    //      "SERVICE DELIVER"                                    
    const QString command ();

    //  Return last received status
    int status ();

    //  Return last received reason
    const QString reason ();

    //  Return last received address
    const QString address ();

    //  Return last received sender
    const QString sender ();

    //  Return last received subject
    const QString subject ();

    //  Return last received content
    QZmsg * content ();

    //  Return last received tracker
    const QString tracker ();

    //  Enable verbose tracing (animation) of state machine activity.
    void setVerbose (bool verbose);

    //  Self test of this class.
    static void test (bool verbose);

    mlm_client_t *self;
};
#endif //  Q_MLM_CLIENT_H
/*
################################################################################
#  THIS FILE IS 100% GENERATED BY ZPROJECT; DO NOT EDIT EXCEPT EXPERIMENTALLY  #
#  Read the zproject/README.md for information about making permanent changes. #
################################################################################
*/
