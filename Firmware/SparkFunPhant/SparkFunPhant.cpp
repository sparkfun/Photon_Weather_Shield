/**
 * Phant.cpp
 *
 *             .-.._
 *       __  /`     '.
 *    .-'  `/   (   a \
 *   /      (    \,_   \
 *  /|       '---` |\ =|
 * ` \    /__.-/  /  | |
 *    |  / / \ \  \   \_\  jgs
 *    |__|_|  |_|__\
 *    never   forget.
 *
 * Original Author: Todd Treece <todd@sparkfun.com>
 * Edited for Particle by: Jim Lindblom <jim@sparkfun.com>
 *
 * Copyright (c) 2014 SparkFun Electronics.
 * Licensed under the GPL v3 license.
 *
 */

//
//   Copyright (C) 2014,2015 Menlo Park Innovation LLC
//
//   menloparkinnovation.com
//   menloparkinnovation@gmail.com
//
//   Snapshot License
//
//   This license is for a specific snapshot of a base work of
//   Menlo Park Innovation LLC on a non-exclusive basis with no warranty
//   or obligation for future updates. This work, any portion, or derivative
//   of it may be made available under other license terms by
//   Menlo Park Innovation LLC without notice or obligation to this license.
//
//   There is no warranty, statement of fitness, statement of
//   fitness for any purpose, and no statements as to infringements
//   on any patents.
//
//   Menlo Park Innovation has no obligation to offer support, updates,
//   future revisions and improvements, source code, source code downloads,
//   media, etc.
//
//   This specific snapshot is made available under the following license:
//
//   Same license as the code from SparkFun above so as not to conflict
//   with the original licensing of the code.
//
//     >> Licensed under the GPL v3 license.
//

#include "SparkFunPhant.h"
#include <stdlib.h>

Phant::Phant(String host, String publicKey, String privateKey) {
  _host = host;
  _pub = publicKey;
  _prv = privateKey;
  _params = "";

  _port = 80;
  _responseLength = 0;
  _response[0] = '\0';

  _debug = true;
}

//
// This is exposed to allow runtime setting, such as through
// a Particle.function() call.
//
void
Phant::setDebug(bool value)
{
    _debug = value;
}

void Phant::add(String field, String data) {

  _params += "&" + field + "=" + data;

}


void Phant::add(String field, char data) {

  _params += "&" + field + "=" + String(data);

}


void Phant::add(String field, int data) {

  _params += "&" + field + "=" + String(data);

}


void Phant::add(String field, byte data) {

  _params += "&" + field + "=" + String(data);

}


void Phant::add(String field, long data) {

  _params += "&" + field + "=" + String(data);

}

void Phant::add(String field, unsigned int data) {

  _params += "&" + field + "=" + String(data);

}

void Phant::add(String field, unsigned long data) {

  _params += "&" + field + "=" + String(data);

}

void Phant::add(String field, double data, unsigned int precision) {

  String sd(data, precision);
  _params += "&" + field + "=" + sd;

}

void Phant::add(String field, float data, unsigned int precision) {

  String sf(data, precision);
  _params += "&" + field + "=" + sf;

}

String Phant::queryString() {
  return String(_params);
}

String Phant::url() {

  String result = "http://" + _host + "/input/" + _pub + ".txt";
  result += "?private_key=" + _prv + _params;

  _params = "";

  return result;

}

String Phant::get() {

  String result = "GET /output/" + _pub + ".csv HTTP/1.1\n";
  result += "Host: " + _host + "\n";
  result += "Connection: close\n";

  return result;

}

String Phant::post() {

	String params = _params.substring(1);
	String result = "POST /input/" + _pub + ".txt HTTP/1.1\n";
	result += "Host: " + _host + "\n";
	result += "Phant-Private-Key: " + _prv + "\n";
	result += "Connection: close\n";
	result += "Content-Type: application/x-www-form-urlencoded\n";
	result += "Content-Length: " + String(params.length()) + "\n\n";
	result += params;

	_params = "";
	return result;

}

String Phant::clear() {

  String result = "DELETE /input/" + _pub + ".txt HTTP/1.1\n";
  result += "Host: " + _host + "\n";
  result += "Phant-Private-Key: " + _prv + "\n";
  result += "Connection: close\n";

  return result;

}

//
// Menlo re-written post function.
//
// menloparkinnovation.com
// menloparkinnovation@gmail.com
//
// - Properly handles TCP segment framing and delays
//
// - Properly ensures NUL termination of response buffers
//   before searching them with strstr
//
// - Allow the proper HTTP response code for new item create 201
//
// - Support configurable timeout
//
// - Support configurable debug
//
int Phant::postToPhant(
    unsigned long timeout
    )
{
    int c;
    int retVal;
    unsigned int index;

    TCPClient client;

    //
    // Clear the response buffer as we are starting a new request exchange
    // with the server.
    //
    _response[0] = '\0';
    _responseLength = 0;

    // Generate the headers, URL, and content document
    String postBody = this->post();

    // Attempt to connect to the host
    retVal = client.connect(_host, _port);
    if (retVal == 0) {
        // error connecting

        if (_debug) {
            Serial.print("Phant: Could not connect to server ");
            Serial.print(_host);
            Serial.print(_host);
            Serial.print(" port ");
            Serial.println(_port);
        }

        // ensures we don't leave a half connection hanging
        client.stop();
        return -1;
    }

    if (_debug) {
        Serial.print("Phant: Connected to server ");
        Serial.print(_host);
        Serial.print(" port ");
        Serial.println(_port);
    }

    if (_debug) {
        Serial.println("posting document:");
        Serial.println(postBody);
    }

    // Send the POST
    client.print(postBody);

    if (_debug) {
        Serial.println("Phant: Done posting document body");
    }

    //
    // Now get any received document response
    //
    index = 0;

    while (client.connected() && (client.available() || (timeout-- > 0))) {

        c = client.read();
        if (c == (-1)) {
            // This ensures the timeout value represents a count of milliseconds
            delay(1);
            continue;
        }

        // Leave room for terminating NUL '\0'
        if (index < (sizeof(_response) - 1)) {

            // add to response buffer
            _response[index++] = c;
        }
        else {

            //
            // we continue to drain characters until timeout as to
            // not reset/hang the connection.
            //
        }
    }

    _responseLength = index;
    _response[index] = '\0'; // ensure NUL terminated

    //
    // We are done with the connection, close it now before any
    // serial output so we don't fire off any timeouts
    //
    if (_debug) {
        Serial.println("calling client.stop...");
    }

    client.stop();

    if (index == (sizeof(_response) - 1)) {

        //
        // Caller can test for overflow by
        // getResponseLength() == (getResponseBufferSize() - 1)
        //
        if (_debug) {
            Serial.println("Phant: maximum document size received, truncated");
        }
    }

    if (_debug) {
        Serial.println("Phant: response document before string search:");
        Serial.print("responseLength ");
        Serial.print(_responseLength);
        Serial.println(" responseDocument:");
        Serial.println(_response);
    }

    //
    // search for the 200 or 201 OK responses
    //
    // Note: 201 is the correct response for a new item added, but
    // many servers return generic 200 OK
    //
    if (strstr(_response, "200 OK") || strstr(_response, "201 OK")) {
        retVal = 1;

        //
        // The _response buffer can be examined by the caller
        // to process the response contents.
        //

        if (_debug) {
            Serial.println("Phant: OK response");
            Serial.print("responseLength ");
            Serial.print(_responseLength);
            Serial.println(" responseDocument:");
            Serial.println(_response);
        }
    }
    else if (strstr(_response, "400 Bad Request")) {
        retVal = -1;

        if (_debug) {
            Serial.println("Phant: Bad Request");
            Serial.print("responseLength ");
            Serial.print(_responseLength);
            Serial.println(" responseDocument:");
            Serial.println(_response);
        }
    }
    else {
        // error of some sort
        retVal = -1;
    }

    return retVal;
}
