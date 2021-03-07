'use strict';
var AWS = require('aws-sdk');

var iotdata = new AWS.IotData({ endpoint: 'XXXXXXXXXXXX-ats.iot.us-east-1.amazonaws.com' });
const tableName = 'SensorTable';
const docClient = new AWS.DynamoDB.DocumentClient();

let deviceActions = {
  'power on': '1',
  'power off': '0',
  'read': 'read'
}

let units = {
  'temperature': 'degrees fahrenheit',
  'gas': 'parts per million',
  'humidity': 'percent'
}

const buildResponse = (alexaResponse, endConversation) => {
  return {
    version: '1.0',
    response: {
      outputSpeech: {
        type: 'PlainText',
        text: alexaResponse,
      },
      shouldEndSession: endConversation,
    },
  };
}

async function dbRead(params) {
  let promise = docClient.scan(params).promise();
  let result = await promise;
  let data = result.Items;
  if (result.LastEvaluatedKey) {
    params.ExclusiveStartKey = result.LastEvaluatedKey;
    data = data.concat(await dbRead(params));
  }
  return data;
}

module.exports.sensorHandler = async (event, context, callback) => {

  var response = buildResponse('Please ask for a sensor and an action', true);

  if (!(event.request.hasOwnProperty('intent')) || !(event.request.intent).hasOwnProperty('slots')) {
    return response;
  }

  var deviceName = event.request.intent.slots.Sensor.value;
  var action = deviceActions[event.request.intent.slots.Action.value];

  console.log(deviceName, action);

  if (action === 'read') {
    //READ FROM DYNAMODB

    let tableParams = { TableName: tableName };

    response = await dbRead(tableParams).then(data => {
      console.log(data)
      let readValue = data[data.length - 1]['data'][deviceName];

      return buildResponse(`The ${deviceName} is ${readValue} ${units[deviceName]}`, true);
    }).catch(error => {
      console.log(error)
      return buildResponse('Alexa could not read', true)
    });

  } else {
    //WRITE TO AWS IOT
    var params = {
      topic: deviceName,
      payload: action,
      qos: 0
    };

    response = await iotdata.publish(params).promise()
      .then(data => {
        return buildResponse(`Published ${action} to ${deviceName}`, true)
      }).catch(error => {
        console.log(error)
        return buildResponse('Alexa could not publish', true)
      });
  }

  return response;
};

