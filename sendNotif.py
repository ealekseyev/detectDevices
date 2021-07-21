import smtplib
from sys import argv

gmail_user = 'evansiotdevices@gmail.com'
gmail_password = 'Evaniot20'

sent_from = gmail_user
to = ['6692785306@tmomail.net']
subject = 'Device authenticated'

DAD_MAC = ""
MOM_MAC = ""
VERA_MAC = ""
EVAN_MAC = "0a:1d:85:3b:b1:ee"

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
	else:
		body = argv[1].lower() + " has joined WiFi"
elif argv[2] == "DISC":
	pass

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
