from flask import Flask, jsonify, abort, render_template, request
import json
import UARTcmd
app = Flask(__name__)

@app.route('/')
def hello_world():
    return 'Hello, World!\n'

welcome = "Welcome to 3ESE API!\n"

@app.route('/api/welcome/')
def api_welcome():
    return welcome

@app.route('/api/welcome/<int:index>', methods=['GET', 'POST', 'PUT', 'PATCH', 'DELETE'])
def api_welcome_index(index=None):
    global welcome
    #return welcome[index]
    #return (   json.dumps({"index": index, "val": welcome[index]}),
        #       {"Content-Type": "application/json"})
    if type(index) != int or index < 0 or index > len(welcome):
        abort(404)  # or use "redirect(url_for(<url>))"
        return
    resp = {
            "method":   request.method,
            "url" :  request.url,
            "args": request.args,
            "headers": dict(request.headers),
    }

    if request.method == 'POST':
        resp["POST"] = {"data" : request.get_json(), }
        welcome = resp['POST']['data']

    elif request.method == 'GET':
        if index is None: return jsonify({"val": welcome})
        else:  return jsonify({"index": index, "val": welcome[index]})

    elif request.method == 'PUT':
        welcome = ''.join([welcome[:index], request.data, welcome[index:]])

    elif request.method == 'PATCH':
        if type(request.data) == str:
            welcome[index] = request.data[0]

    elif request.method == 'DELETE':
        if index is None: welcome = 0
        else:
            val = welcome[index]
            welcome = welcome[:index] + welcome[index+1:]
        return jsonify({"index": index, "val": val, "sentence" : welcome})


@app.errorhandler(404)
def page_not_found(error):
    return render_template('page_not_found.html'), 404


@app.route('/api/request/', methods=['GET', 'POST'])
@app.route('/api/request/<path>', methods=['GET','POST'])
def api_request(path=None):
    resp = {
            "method":   request.method,
            "url" :  request.url,
            "path" : path,
            "args": request.args,
            "headers": dict(request.headers),
    }

    if request.method == 'POST':
        resp["POST"] = {
                "data" : request.get_json(),
                }
    return jsonify(resp)


# API for sensor
tempMeasArray = []
presMeasArray = []
#temperature
@app.route('/temp/', methods=['GET','POST'])
def tempHandling():
    if request.method == 'POST':
        tempMeasArray.append(UARTcmd.sendCMD('GET_T'))
        return jsonify({"type": 'temperature',
                "index": x,
                "value": tempMeasArray[x]})

    elif request.method == 'GET':
        return jsonify({
        "type": 'temperature',
        "index": 'all',
        "value": '\n'.join(tempMeasArray)}
        )

#tempX
@app.route('/temp/<int:x>', methods=['GET','POST'])
def tempXHandling(x=None):
    if request.method == 'DELETE':
        return jsonify({"type": 'temperature',
                "index": x,
                "value deleted": tempMeasArray.pop(x)})

    elif request.method == 'GET':
        return jsonify({"type": 'temperature',
                "index": x,
                "value": tempMeasArray[x]})
#pressure
@app.route('/pres/', methods=['GET','POST'])
def presHandling():
    if request.method == 'POST':
        presMeasArray.append(UARTcmd.sendCMD('GET_P'))
        return jsonify({"type": 'pressure',
                "index": x,
                "value": presMeasArray[x]})

    elif request.method == 'GET':
        return jsonify({
        "type": 'presssure',
        "index": 'all',
        "value": '\n'.join(presMeasArray)}
        )

#presX
@app.route('/pres/<int:x>', methods=['GET','POST'])
def presXHandling(x=None):
    if request.method == 'DELETE':
        return jsonify({"type": 'pressure',
                "index": x,
                "value deleted": presMeasArray.pop(x)})

    elif request.method == 'GET':
        return jsonify({"type": 'pressure',
                "index": x,
                "value": presMeasArray[x]})

#scale and angle
@app.route('/scale/', methods=['GET'])
def scaleHandling():
    return jsonify({
        "type": 'scale',
        "value": UARTcmd.sendCMD('GET_K')}
        )

@app.route('/scale/<int:x>', methods=['POST'])
def scaleXHandling(x=None):
    return jsonify({
        "type": 'scale',
        "callback": UARTcmd.sendCMD(f'SET_K={x}')}
        )

@app.route('/angle/', methods=['GET'])
def angleHandling():
    return jsonify({
        "type": 'angle',
        "value": UARTcmd.sendCMD('GET_A')}
        )
