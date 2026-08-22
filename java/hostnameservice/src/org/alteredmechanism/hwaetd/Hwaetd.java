package org.alteredmechanism.hwaetd;

import java.net.*;
import java.util.logging.Logger;

import static java.util.logging.Level.SEVERE;

public class Hwaetd {
    
    public static final int PORT = 4140;
    public static final int RESPONSE_PORT = 4141;

    private static final Logger logger = Logger.getLogger(Hwaetd.class.getName());

    /**
     * Executions start here.
     * 
     * @param args Command line arguments
     */
    public static void main(String[] args) {
        try (DatagramSocket serverSock = new DatagramSocket(PORT)) {
            String myHostname = InetAddress.getLocalHost().getHostName();
            String myIP = InetAddress.getLocalHost().getHostAddress();
            logger.info("Hostname: " + myHostname + ", IP: " + myIP);

            byte[] incomingData = new byte[1024];
            DatagramPacket incomingPacket = new DatagramPacket(incomingData, incomingData.length);
            
            try (DatagramSocket outgoingSocket = new DatagramSocket()) {
                byte[] outgoingData = (myHostname + " " + myIP).getBytes();
                DatagramPacket outgoingPacket = new DatagramPacket(outgoingData, outgoingData.length);
                outgoingPacket.setData(outgoingData);
                outgoingPacket.setPort(RESPONSE_PORT);

                //noinspection InfiniteLoopStatement
                while (true) {
                    serverSock.receive(incomingPacket);
                    InetAddress returnAddress = incomingPacket.getAddress();
                    String dataReceived = new String(incomingPacket.getData(), 0, incomingPacket.getLength());
                    logger.info("Received request from " + returnAddress.toString() + ": " + dataReceived);
                    if (dataReceived.equals("*") || dataReceived.isEmpty() || dataReceived.equalsIgnoreCase(myHostname)) {
                        outgoingPacket.setAddress(returnAddress);
                        outgoingSocket.send(outgoingPacket);
                    }
                    logger.info("Sent response to " + returnAddress + ": " + new String(outgoingPacket.getData()));
                }
            }
        }
        catch (Exception e) {
            logger.log(SEVERE, "Server error", e);
        }

    }

}
