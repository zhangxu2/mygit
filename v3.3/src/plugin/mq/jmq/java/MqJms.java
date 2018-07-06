package mqjms;

import javax.jms.*;
import javax.naming.*;
import com.ibm.mq.jms.*;
import com.ibm.msg.client.jms.*;
import java.io.IOException;
import java.util.Date;
import java.util.logging.FileHandler;
import java.util.logging.Formatter;
import java.util.logging.Level;
import java.util.logging.LogRecord;
import java.util.logging.Logger;

class LogFormatter extends Formatter {
	@Override
	public String format(LogRecord record) {
		Date date = new Date();
		String sDate = date.toString();
		return "[" + sDate + "]" + " [" + record.getLevel() + "] " + record.getMessage() + "\n";
	}
}

class MqJms {

	private QueueConnection conn = null;
	private QueueSession session = null;
	private Message message = null;
	private Destination senddest = null;
	private Destination recvdest = null;
	private MessageConsumer consumer = null;
	private MessageProducer producer = null;
	public static int port = 0;
	public static String ip = null;
	public static String qmgr = null;
	public static String jSendQueue = null;
	public static String jRecvQueue = null;
	private static Logger log = Logger.getLogger(MqJms.class.getName());

	public MqJms(int port, String ip,String qmgr, String jsendQ, String jrecvQ) {

		this.ip = ip;
		this.port = port;
		this.qmgr = qmgr;
		this.jSendQueue = jsendQ;
		this.jRecvQueue = jrecvQ;
	}
	
	private int initLog() {
		log.setLevel(Level.ALL);
		FileHandler fileHandler;
		String work = System.getenv("SWWORK");
		
		try {
			fileHandler = new FileHandler(work + "/log/syslog/" + qmgr + ".log");
		} catch (IOException e) {
			return -1;
		}

		fileHandler.setLevel(Level.ALL);
		fileHandler.setFormatter(new LogFormatter());
		log.addHandler(fileHandler);
		log.setUseParentHandlers(false);

		return 0;
	}
	
	public int init() {

		MQQueueConnectionFactory cf = new MQQueueConnectionFactory();
		if (initLog() < 0) {
			System.out.println("Init Log error!");
			return -1;
		}

		cf.setHostName(ip);
		try {
	 		cf.setPort(port);
		} catch(JMSException je) {

			return -1;
		}

		try {
			cf.setTransportType(JMSC.MQJMS_TP_CLIENT_MQ_TCPIP);
		} catch(JMSException je) {
			return -1;
		}
			
		try { 	
			cf.setQueueManager(qmgr);
		} catch(JMSException je) {
			return -1;
		}
			   
		try {
			cf.setChannel("SYSTEM.DEF.SVRCONN");
		} catch(JMSException je) {
			return -1;
		}
			   
			
		if ((jSendQueue == null && jRecvQueue == null) || qmgr == null) {

			return -1;
		}
		
		try {
			conn = cf.createQueueConnection();
		} catch(JMSException je) {
			return -1;
		}
		
		try {
			session = conn.createQueueSession(false, Session.AUTO_ACKNOWLEDGE);
		} catch(JMSException ne) {
			return -1;
		}

		if (jSendQueue != null) {
			try {
				String sendqueue = null;
				sendqueue = String.format("queue:///%s", jSendQueue);
				senddest = session.createQueue(sendqueue);
			} catch(JMSException ne) {
				return -1;
			}
		}
		
		if (jRecvQueue != null) {
			try{
				String recvqueue = null;
				recvqueue = String.format("queue:///%s", jRecvQueue);
				recvdest = session.createQueue(recvqueue);
			}catch(JMSException ne){
				return -1;
			}
		}
		log.info("init success!");
		
		return 0;
	}

	public int initSend() {
		
		if (senddest == null) {
			return -1;
		}
		
		try {
			producer = session.createProducer(senddest);
		} catch (JMSException je) {
			return -1;
		}

		return 0;
	}
	
	public int initRecv() {

		if (recvdest == null) {
			return -1;
		}

		try {
			consumer = session.createConsumer(recvdest);
			conn.start();
		} catch (JMSException je) {
			return -1;
		}

		return 0;
	}

	public int sendMessage(String sendMsg) {

		try {
			message = session.createTextMessage(sendMsg);
			producer.send(message);
		} catch (JMSException je) {
			return -1;
		}
			
		return 0;
	}

	public String recvMessage(int timeout) {

		String	text = null;
		try {
			if (timeout > 0) {
				message = consumer.receive(timeout);
			}
			else {
				message = consumer.receive();
			}
			if (message instanceof TextMessage) {
				text = ((TextMessage) message).getText();
			}
		} catch (JMSException je) {
			return null;
		}
		return text;
	}
}
