
commandlist1 = ["takeoff", "take", "land", "finish", "quit", "offboard", "onboard"]
commandlist2 = ["turn", "go"]
forwardlist = ["forward", "straight", "front"]
backwardlist = ["backward", "back", "behind"]
leftwardlist = ["left", "leftside", "counterclockwise"]
rightwardlist = ["right", "rightside", "clockwise"]
uplist = ["up", "upward"]
downlist = ["down", "downward"]
distancemeasure = ["meter", "meters", "feet", "feets", "yard","yards"]
timemeasure =["second","seconds","minute","minutes","hour","hours"]
degreemeasure = ["degree", "degrees", "radian"]

commandlist = commandlist1 + commandlist2
directionlist = forwardlist + backwardlist + leftwardlist + rightwardlist



def is_number(number):
	try:
		float(number)
		return True
	except ValueError:
		return False

def find_vel(wordlist):
	for word in wordlist:
		if is_number(word):
			numbindex = wordlist.index(word)
			if (wordlist[numbindex+1] in distancemeasure) and (wordlist[numbindex+2] == "per") and (wordlist[numbindex+3] in timemeasure):
				return (True, word)
		else:
			continue
	return (False, "No velocity component")

def find_distance(wordlist):
	for word in wordlist:
		if is_number(word):
			numbindex = wordlist.index(word)
			if (wordlist[numbindex+1] in distancemeasure) and (wordlist[numbindex+2] != "per"):
				return (True, word)
		else:
			continue
	return (False, "No distance component")

def find_time(wordlist):
	for word in wordlist:
		if is_number(word):
			numbindex = wordlist.index(word)
			if (wordlist[numbindex+1] in timemeasure):
				return (True, word)
		else:
			continue
	return (False, "No distance component")

def find_direction(wordlist):
	for word in wordlist:
		if word in directionlist:
			return (True, word)
		else:
			continue
	return (False, "No direction component")

def find_gps(wordlist):
	for word in wordlist:
		if is_number(word):
			numbindex = wordlist.index(word)
			if is_number(wordlist[numbindex+1]) and is_number(wordlist[numbindex+2]):
				return (True, wordlist[numbindex:numbindex+3])
		else:
			continue
	return (False, "No gps component")

def find_degree(wordlist):
	for word in wordlist:
		if is_number(word):
			numbindex = wordlist.index(word)
			if (wordlist[numbindex+1] in degreemeasure):
				return (True, word)
		else:
			continue
	return (False, "No degree component")



def find_intent(wordlist):
	for word in wordlist:
		if word in commandlist:
			return (True, word)
	return (False, "No command component")


def make_message(voice_text):
	voice_text.lower()
	wordlist = voice_text.split()
	wordlist.append("Trash")
	wordlist.append("Trash")
	wordlist.append("Trash")

	intentresult, intent= find_intent(wordlist)
	if not intentresult:
		print intent
		return (False, "Wrong format")

	if intent in commandlist1:
		if intent =="take":
			message = "takeoff"
		else:	
			message = intent
		result = True

	elif intent in commandlist2:

		if intent == "go":
			gpsresult, gps = find_gps(wordlist)
			velresult, vel = find_vel(wordlist)
			dirresult, direction = find_direction(wordlist)
			disresult, dis = find_distance(wordlist)
			
			if gpsresult:
				message = ' '.join(["navgoto", gps[0], gps[1], gps[2], '0'])
				result = True
			elif disresult:
				if not dirresult:
					return (False, "Wrong direction")
				initvel = '3'
				if velresult:
					initvel = vel
				if direction in forwardlist:
					message = ' '.join(['goto', dis, '0', '0', initvel])
				elif direction in backwardlist:
					minus = ''.join(['-',dis])
					message = ' '.join(['goto',minus, '0', '0', initvel])
				elif direction in leftwardlist:
					minus = ''.join(['-',dis])
					message = ' '.join(['goto', '0', minus, '0', initvel])
				elif direction in rightwardlist:
					message = ' '.join(['goto', '0', dis, '0', initvel])
				elif direction in downlist:
					message = ' '.join(['goto', '0', '0', dis, initvel])
				elif direction in uplist:
					minus = ''.join(['-',dis])
					message = ' '.join(['goto', '0', '0', minus, initvel])
				result = True

			elif not disresult and velresult:
				timeresult, time = find_time(wordlist)
				if not dirresult:
					return (False, "Wrong direction")
				initdur = '5'
				if timeresult:
					initdur = time
				if direction in forwardlist:
					message = ' '.join(['velgoto', vel, '0', '0', initdur])
				elif direction in backwardlist:
					minus = ''.join(['-',vel])
					message = ' '.join(['velgoto',minus, '0', '0', initdur])
				elif direction in leftwardlist:
					minus = ''.join(['-',vel])
					message = ' '.join(['velgoto', '0', minus, '0', initdur])
				elif direction in rightwardlist:
					message = ' '.join(['velgoto', '0', vel, '0', initdur])
				elif direction in downlist:
					message = ' '.join(['velgoto', '0', '0', vel, initdur])
				elif direction in uplist:
					minus = ''.join(['-',vel])
					message = ' '.join(['velgoto', '0', '0', minus, initdur])
				result = True

			else:
				result = False
				message = "Not enough componets!"

		elif intent =="turn":

			degresult, degree = find_degree(wordlist)
			if not degresult:
				return (False, "No degree component")

			dirresult, direction = find_direction(wordlist)
			if not dirresult:
				return (False, "Wrong direction")


			if direction in rightwardlist:
				message = ' '.join([intent, degree])
			elif direction in leftwardlist:
				minus = ''.join(['-',degree])
				message = ' '.join([intent, minus])
			else:
				result = False
				message = "Wrong direction"
			result = True

		else:
			result = False
			message = "Wrong command"

	else:
		result = False
		message = "Wrong format"

	wordlist.pop()
	wordlist.pop()
	wordlist.pop()


	return (result, message)







print "Start message factory"

while True:
	command = raw_input("Command: ")

	if command == "quit":
		break


	else:
		result, message = make_message(command)
		if result:
			print "Message: " + message
		else:
			print message


