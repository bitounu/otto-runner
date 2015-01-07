#!/usr/bin/python
from bottle import route, run, template, static_file
import os
import bottle
# bottle.TEMPLATE_PATH.insert(0,'/home/pi/otto-sdk/python/views')
os.chdir('/home/pi/otto-sdk/python/')
@route('/')
def index():
    dirs = os.listdir( "/home/pi/output/" )
    dirs.sort(reverse=True)
    return template('images', files=dirs)

@route('/image/<name:re:gif_[0-9]{4}.gif>')
def callback(name):
    return static_file(name, root='/home/pi/output')

run(host='0.0.0.0', port=80)
