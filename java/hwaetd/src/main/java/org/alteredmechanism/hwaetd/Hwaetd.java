package org.alteredmechanism.hwaetd;

import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.Platform;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
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
    private static final int HOSTNAME_BUFFER_LENGTH = 256;

    private static final Logger logger = Logger.getLogger(Hwaetd.class.getName());

    private interface LibC extends Library {
        int gethostname(byte[] name, int len);
    }

    private static final LibC libc = Native.load(Platform.C_LIBRARY_NAME, LibC.class);

    public static String gethostname() {
        byte[] hostnameBuffer = new byte[HOSTNAME_BUFFER_LENGTH];
        int result = libc.gethostname(hostnameBuffer, hostnameBuffer.length);
        if (result != 0) {
            throw new IllegalStateException("Failed to get hostname: " + Native.getLastError());
        }

        int i = 0;
        while (i < hostnameBuffer.length && hostnameBuffer[i] != 0) {
            i++;
        }
        return new String(hostnameBuffer, 0, i);
    }

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
        String myHostname = gethostname();
        System.out.println("My hostname: " + myHostname + ", IP: " + myIP);
        try (DatagramSocket serverSock = new DatagramSocket(PORT)) {

            byte[] incomingDataBuffer = new byte[1024];
            DatagramPacket incomingPacket = new DatagramPacket(incomingDataBuffer, incomingDataBuffer.length);

            try (DatagramSocket outgoingSocket = new DatagramSocket()) {
                byte[] intlHostname = convertToIntlHostname(myHostname);
                DatagramPacket outgoingPacket = new DatagramPacket(intlHostname, intlHostname.length, new InetSocketAddress(RESPONSE_PORT));

                //noinspection InfiniteLoopStatement
                while (true) {
                    serverSock.receive(incomingPacket);
                    // Convert incoming ISO-8859-1 (Latin-1) encoded bytes to a Java String (UTF-16).
                    InetAddress returnAddress = incomingPacket.getAddress();
                    String incomingData = new String(incomingPacket.getData(), 0, incomingPacket.getLength(), StandardCharsets.ISO_8859_1);
                    if (incomingData.equalsIgnoreCase(MAGIC_WORD)) {
                        logger.info("Received request from " + returnAddress.getHostAddress() + ": " + incomingData);
                        outgoingPacket.setAddress(returnAddress);
                        outgoingSocket.send(outgoingPacket);
                        logger.info("Sent response to " + returnAddress.getHostAddress() + ": " + new String(outgoingPacket.getData(), StandardCharsets.UTF_8));
                    } else {
                        String bytes = convertToHex(incomingDataBuffer, incomingPacket.getLength());
                        String chars = convertToHex(incomingData);
                        logger.warning("Received invalid request from " + returnAddress.getHostAddress() + ": " + incomingData + " (hex: " + bytes + ") + (chars: " + chars + ")");
                        logger.warning("Expected: " + MAGIC_WORD + " (chars: " + convertToHex(MAGIC_WORD) + ")");
                    }
                }
            } catch (IOException ex) {
                throw new RuntimeException(ex);
            }
        } catch (Exception e) {
            logger.log(SEVERE, "Server error", e);
        }
    }

    private static byte[] convertToIntlHostname(String myHostname) {
        String idn = IDN.toASCII(myHostname);
        return idn.getBytes(StandardCharsets.US_ASCII);
    }

    private static String convertToHex(String incomingData) {
        StringBuilder sb = new StringBuilder();
        byte[] bytes = incomingData.getBytes(StandardCharsets.UTF_8);
        for (byte b : bytes) {
            sb.append(String.format("$%02X ", b));
        }
        if (!sb.isEmpty()) {
            sb.deleteCharAt(sb.length() - 1);
        }
        return sb.toString();
    }

    private static String convertToHex(byte[] incomingDataBuffer, int length) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < length; i++) {
            sb.append(String.format("$%02X ", incomingDataBuffer[i]));
        }
        if (!sb.isEmpty()) {
            sb.deleteCharAt(sb.length() - 1);
        }
        return sb.toString();
    }
}
