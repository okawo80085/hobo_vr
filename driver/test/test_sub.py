import zmq

aContext = zmq.Context()                                          # .new Context
aSUB     = aContext.socket( zmq.SUB )                             # .new Socket
aSUB.connect( "tcp://127.0.0.1:6969" )                            # .connect
aSUB.setsockopt( zmq.LINGER, 0 )                                  # .set ALWAYS!
aSUB.setsockopt( zmq.SUBSCRIBE, "DriverSender".encode() )                              # .set T-filter

MASK = "INF: .recv()-ed this:[{0:}]\n:     waited {1: > 7d} [us]"

while True:
    try:
        print('receiving')
        print (aSUB.recv(20))
    except ( KeyboardInterrupt, SystemExit ):
        pass
        break
pass
aSUB.close()                                                     # .close ALWAYS!
aContext.term()                                                  # .term  ALWAYS!