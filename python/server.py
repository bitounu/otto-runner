#!/usr/bin/python
from bottle import route, run, template, static_file
import os

@route('/images')
def index():
    dirs = os.listdir( "/home/pi/output/" )
    dirs.sort(reverse=True)
    return template('images', files=dirs)

@route('/image/<name:re:gif_[0-9]{4}.gif>')
def callback(name):
    return static_file(name, root='/home/pi/output')

run(host='192.168.2.14', port=8080)
