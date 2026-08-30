package org.alteredmechanism.hwaetd;

import java.io.IOException;
import java.net.*;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Enumeration;
import java.util.List;
import java.util.logging.Logger;

import static java.util.logging.Level.SEVERE;

public class Hwaetd {

    public static final int PORT = 4140;
    public static final int RESPONSE_PORT = 4141;
    public static final String MAGIC_WORD = "Hwæt";

    private static final Logger logger = Logger.getLogger(Hwaetd.class.getName());

    public static List<Inet4Address> getIpAddresses() throws SocketException {
        List<Inet4Address> ipAddresses = new ArrayList<>();
        // Get all network interfaces on the machine
        Enumeration<NetworkInterface> interfaces = NetworkInterface.getNetworkInterfaces();

        for (NetworkInterface netIf : Collections.list(interfaces)) {
            // Skip loopback, inactive, or virtual interfaces
            if (netIf.isLoopback() || !netIf.isUp() || netIf.isVirtual()) {
                continue;
            }

            // Get all IP addresses bound to this interface
            Enumeration<InetAddress> addresses = netIf.getInetAddresses();
            for (InetAddress addr : Collections.list(addresses)) {
                // Check if it's an IPv4 address (change to Inet6Address for IPv6)
                if (addr instanceof java.net.Inet4Address) {
                    ipAddresses.add((Inet4Address) addr);
                }
            }
        }
        return ipAddresses;
    }

    /**
     * Executions start here.
     *
     * @param args Command line arguments
     */
    public static void main(String[] args) throws SocketException {
        List<Inet4Address> myIpAddresses = getIpAddresses();
        String myIP = myIpAddresses.getFirst().getHostAddress();
        String myHostname = myIpAddresses.getFirst().getHostName();
        System.out.println("Hostname: " + myHostname + ", IP: " + myIP);
        try (DatagramSocket serverSock = new DatagramSocket(PORT)) {

            byte[] incomingData = new byte[1024];
            DatagramPacket incomingPacket = new DatagramPacket(incomingData, incomingData.length);

            try (DatagramSocket outgoingSocket = new DatagramSocket()) {
                byte[] outgoingData = (myHostname + " " + myIP).getBytes();
                DatagramPacket outgoingPacket = new DatagramPacket(outgoingData, outgoingData.length);
                outgoingPacket.setData(outgoingData);
                outgoingPacket.setPort(RESPONSE_PORT);

                //noinspection InfiniteLoopStatement
                while (true) {
                    InetAddress.getLocalHost().getHostName();
                    serverSock.receive(incomingPacket);
                    ((InetSocketAddress) incomingPacket.getSocketAddress()).getHostName();
                    InetAddress returnAddress = incomingPacket.getAddress();
                    String dataReceived = new String(incomingPacket.getData(), 0, incomingPacket.getLength());
                    logger.info("Received request from " + returnAddress.getHostName() + ": " + dataReceived);
                    if (dataReceived.equalsIgnoreCase(MAGIC_WORD)) {
                        outgoingPacket.setAddress(returnAddress);
                        outgoingSocket.send(outgoingPacket);
                    }
                    logger.info("Sent response to " + returnAddress.getHostName() + ": " + new String(outgoingPacket.getData()));
                }
            } catch (IOException ex) {
                throw new RuntimeException(ex);
            }
        } catch (Exception e) {
            logger.log(SEVERE, "Server error", e);
        }
    }
}
