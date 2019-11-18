import configparser
import os
parentdir=os.path.split(os.path.realpath(__file__))[0]
print(os.path.realpath(__file__))
path=os.path.join(parentdir, 'config.conf')
cf = configparser.ConfigParser()


cf.read(path)
ip = cf.get('baseconf', 'presenter_server_ip')
port = cf.get('baseconf', 'presenter_server_port')
print(cf.sections())
