from socket import *
from select import *
from time import ctime
import sys
import io
import os
import time


# size for each Host and Buffer
HOST = '0.0.0.0'
PORT = 8080
BUFSIZE = 1024
ADDR = (HOST,PORT)



def set_socketServer():

	# make socket descripter
    serverSocket = socket(AF_INET, SOCK_STREAM)

	# bind server
    serverSocket.bind((HOST,PORT))

	# listening
    serverSocket.listen(1)

    print "Waiting for connection ..."
    client_socket, client_address = serverSocket.accept()

    try:
        print "Connection from:", client_address
        data = client_socket.recv(1024)
        print "Receive data"
        client_socket.send(data)

    except:
        client_socket.close()
        print "Connection faild"

    return client_socket        


def transcribe_file(speech_file):
    #Transcribe the given audio file.
    try:
        from google.cloud import speech
    except ImportError:
        print("Error speech import error")
        exit(255)
    from google.cloud.speech import enums
    from google.cloud.speech import types
    client = speech.SpeechClient()

    with io.open(speech_file, 'rb') as audio_file:
        content = audio_file.read()

    audio = types.RecognitionAudio(content=content)
    config = types.RecognitionConfig(
        encoding=enums.RecognitionConfig.AudioEncoding.LINEAR16,
        sample_rate_hertz=16000,
        language_code = 'en_US',
        speech_contexts = [speech.types.SpeechContext(
            phrases = ['dron', 'takeoff', 'land','launch','return to launch'],
        )],
    )

    response = client.recognize(config, audio)
    transcript = ""
    for result in response.results:
        print('Transcript: {}' .format(result.alternatives[0].transcript))
        transcript = transcript + result.alternatives[0].transcript

    return transcript



def voice_record(duration):
    print ("Record for 5 seconds")
    os.system('arecord -t raw -c 1 -d %s -f S16_LE -r 16000 temprec.raw' %duration)       
    speech_file = os.path.join(os.path.dirname(__file__),'temprec.raw')
    return speech_file



def sendMsg_Websocket(socket, message):
	socket.send(message)

def make_message(voice_text):


    commandlist1 = ["takeoff", "take", "land", "finish", "quit", "offboard", "onboard"]
    commandlist2 = ["turn", "go", "look"]
    forwardlist = ["forward", "straight", "front"]
    backwardlist = ["backward", "back", "behind"]
    leftwardlist = ["left", "leftside", "counterclockwise"]
    rightwardlist = ["right", "rightside", "clockwise"]
    uplist = ["up", "upward"]
    downlist = ["down", "downward"]
    result = True

    voice_text.lower()
    msglist = voice_text.split(' ')

    if msglist[0] in commandlist1:
        if msglist[0] == "take":
            message = "takeoff"
        else:   
            message = msglist[0]

    elif msglist[0] in commandlist2:
        if msglist[0] == "go" and len(msglist) >= 3:
            if msglist[1] in forwardlist:
                message = ' '.join([msglist[0], msglist[2], '0', '0', '3'])
            elif msglist[1] in backwardlist:
                minus = ''.join(['-',msglist[2]])
                message = ' '.join([msglist[0], minus, '0', '0', '3'])
            elif msglist[1] in leftwardlist:
                minus = ''.join(['-',msglist[2]])
                message = ' '.join([msglist[0], '0', minus, '0', '3'])
            elif msglist[1] in rightwardlist:
                message = ' '.join([msglist[0], '0', msglist[2], '0', '3'])
            elif msglist[1] in downlist:
                message = ' '.join([msglist[0], '0', '0', msglist[2], '1'])
            elif msglist[1] in uplist:
                minus = ''.join(['-',msglist[2]])
                message = ' '.join([msglist[0], '0', '0', minus, '1'])
            else:
                message = "Wrong direction!"
                result = False

        elif msglist [0] == "turn" and len(msglist) >= 3:
            if msglist[1] in rightwardlist:
                message = ' '.join([msglist[0], msglist[2]])
            elif msglist[1] in leftwardlist:
                minus = ''.join(['-',msglist[2]])
                message = ' '.join([msglist[0], minus])
            else:
                message = "Wrong direction!"
                result = False
        else:
            message = "Wrong format!"
            result = False

    else:
        message = "Wrong format!"
        result = False

    return (result, message)


	

print ("Start Message Sending Program!")

helpmessage = """
v    Vocie recording and send message
d    Change voice recording time
t    Typing command manualy
h    help
q    quit the program
"""
duration = '5'

client_socket = set_socketServer()

while True:
    opt = raw_input("input the option: ")
    

    if opt == 'v':
        speech_file = voice_record(duration)
        text = transcribe_file(speech_file)
        result, message = make_message(text)
        if result:    
            sendMsg_Websocket(client_socket, message)
            peedback = client_socket.recv(1024)
            print peedback
        else:
            print message
         
    elif opt == 't':
        text = raw_input("manual command: ")
        result, message = make_message(text)
        if result:    
            sendMsg_Websocket(client_socket, message)
            peedback = client_socket.recv(1024)
            print peedback

        else:
            print message


    elif opt == 'h':
        print (helpmessage)


    elif opt == 'q':
        print ("Finish the program!")
        sendMsg_Websocket(client_socket, "finish")
        sleep(2)
        try:
            client_socket.close()
            print "program finish!"
            break
        except:
            print ("Socket close failed!")
            exit()


    else:
        print ("Wrong option. please try again")
        print (helpmessage)
        
