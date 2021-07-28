from mac_vendor_lookup import MacLookup
from datetime import datetime
import smtplib
from sys import argv
import time

gmail_user = ''
gmail_password = ''

sent_from = gmail_user
to = ['']
subject = 'Device authenticated'

DAD_MAC = ""
MOM_MAC = ""
VERA_MAC = ""
EVAN_MAC = "0a:1d:85:3b:b1:ee"

BLACKLIST = ["a4:4e:31:d3:5a:a8"]

body = ""
if argv[2] == "CONN":
	if argv[1].lower() == DAD_MAC:
		body = "Dad's iPhone has joined WiFi"
	elif argv[1].lower() == MOM_MAC:
		body = "Mom's iPhone has joined WiFi"
	elif argv[1].lower() == VERA_MAC:
		body = "Vera's iPhone has joined WiFi"
	elif argv[1].lower() == EVAN_MAC:
		body = "Welcome home Evan!"
	elif argv[1].lower() in BLACKLIST:
		exit()
	else:
		try:
			body = argv[1].lower() + " (" + ("ISAPPL" if MacLookup().lookup(argv[1]) == 'Apple, Inc.' else argv[3]) + ") has joined WiFi"
		except:
			body = argv[1].lower() + " ("  + argv[3] + ") has joined WiFi"
elif argv[2] == "DISC":
	exit()

now = datetime.now()
body += "\n" + now.strftime("%H:%M:%S")

email_text = """\
From: %s
To: %s
Subject: %s

%s
""" % (sent_from, ", ".join(to), subject, body)

try:
    server = smtplib.SMTP_SSL('smtp.gmail.com', 465)
    server.ehlo()
    server.login(gmail_user, gmail_password)
    server.sendmail(sent_from, to, email_text)
    server.close()

    print('Email sent!')
except:
    print('Something went wrong...')
