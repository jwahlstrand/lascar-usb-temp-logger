#!/usr/bin/env python

from influxdb import InfluxDBClient
from ConfigParser import SafeConfigParser
import datetime,os,time,errno,json,urllib2

# mostly copied from https://github.com/JQIamo/RPi-Temp-Humidity-Monitor/blob/master/scripts/readInfluxTemp.py

## Load CONFIG stuff
parser = SafeConfigParser({'location':'/dev/shm/missedTempLogs'})
parser.read('/home/wahlstrj/build/lascar-usb-temp-logger/templogger.conf')

data=[]

current_time = str(datetime.datetime.utcnow())

key = parser.get('weather','key')
z = parser.get('weather','zip')
url = 'http://api.wunderground.com/api/' + key + '/geolookup/conditions/q/PA/' + str(z) + '.json'

f = urllib2.urlopen(url)
json_string = f.read()
parsed_json = json.loads(json_string)
temp = parsed_json['current_observation']['temp_f']
dewpoint = parsed_json['current_observation']['dewpoint_f']
f.close()

data += [{"measurement": "temp", "time": current_time, "tags": {"room": "Outside"}, "fields": { "value": temp} }]
data += [{"measurement": "dewpoint", "time": current_time, "tags": {"room": "Outside"}, "fields": { "value": dewpoint} }]

if data:
	try:
		influx_url = parser.get('influx', 'url')
		influx_port = parser.get('influx', 'port')
		influx_user = parser.get('influx', 'username')
		influx_pwd = parser.get('influx', 'password')
		influx_db = parser.get('influx', 'database')
		client = InfluxDBClient(influx_url, influx_port, influx_user, influx_pwd, influx_db)
		client.write_points(data)
	except Exception, e:
		print e
		missedDirectory = parser.get('missed','location')
		try:
			os.makedirs(missedDirectory)
		except OSError as exception:
			if exception.errno != errno.EEXIST:
				raise
		saveFilename = "%d-missedTemps.json" % time.time()
		savePath = os.path.join(missedDirectory, saveFilename)
		print "Attempting to save reading to %s" % savePath
		with open(savePath,'w') as outfile:
			 json.dump(data, outfile)
