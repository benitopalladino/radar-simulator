import javax.swing.*;
import java.awt.*;
import java.io.*;
import java.net.Socket;
import java.util.*;
import java.util.List;

public class RadarClient {

    record Contact(int id, double range, double azimuth, double amplitude) {}

    static class RadarPanel extends JPanel {
        private final Deque<List<Contact>> scans = new ArrayDeque<>();
        private int scanNo = 0;

        void addScan(int n, List<Contact> contacts) {
            scans.addFirst(contacts);
            while (scans.size() > 5) scans.removeLast();
            scanNo = n;
            repaint();
        }

        @Override
        protected void paintComponent(Graphics g) {
            super.paintComponent(g);
            Graphics2D g2 = (Graphics2D) g;
            g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);

            setBackground(Color.BLACK);
            int cx = getWidth() / 2, cy = getHeight() / 2;
            int radius = Math.min(getWidth(), getHeight()) / 2 - 30;

            g2.setColor(new Color(0, 120, 0));
            for (int km = 25; km <= 100; km += 25) {
                int r = radius * km / 100;
                g2.drawOval(cx - r, cy - r, 2 * r, 2 * r);
                g2.drawString(km + " km", cx + 4, cy - r - 2);
            }
            g2.drawLine(cx - radius, cy, cx + radius, cy);
            g2.drawLine(cx, cy - radius, cx, cy + radius);
            g2.drawString("N", cx - 4, cy - radius - 12);

            int age = 0;
            for (List<Contact> scan : scans) {
                int alpha = Math.max(60, 255 - age * 50);
                g2.setColor(new Color(0, 255, 0, alpha));
                for (Contact c : scan) {
                    double rad = Math.toRadians(c.azimuth());
                    int r = (int) (radius * c.range() / 100.0);
                    int x = cx + (int) (r * Math.sin(rad));
                    int y = cy - (int) (r * Math.cos(rad));
                    g2.fillOval(x - 4, y - 4, 8, 8);
                }
                age++;
            }

            g2.setColor(Color.WHITE);
            int current = scans.isEmpty() ? 0 : scans.peekFirst().size();
            g2.drawString("Scan " + scanNo + " - contatti rilevati: " + current, 10, 20);
        }
    }

    public static void main(String[] args) {
        JFrame frame = new JFrame("Radar Display");
        RadarPanel panel = new RadarPanel();
        frame.add(panel);
        frame.setSize(700, 700);
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.setVisible(true);

        Thread reader = new Thread(() -> {
            try (Socket socket = new Socket("localhost", 5050);
                 BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()))) {
                String line;
                List<Contact> current = null;
                int n = 0;
                while ((line = in.readLine()) != null) {
                    if (line.startsWith("SCAN ")) {
                        n = Integer.parseInt(line.substring(5).trim());
                        current = new ArrayList<>();
                    } else if (line.equals("END")) {
                        if (current != null) {
                            final List<Contact> done = current;
                            final int scanNumber = n;
                            SwingUtilities.invokeLater(() -> panel.addScan(scanNumber, done));
                        }
                        current = null;
                    } else if (current != null) {
                        String[] p = line.split(",");
                        if (p.length == 4) {
                            current.add(new Contact(Integer.parseInt(p[0]), Double.parseDouble(p[1]),
                                    Double.parseDouble(p[2]), Double.parseDouble(p[3])));
                        }
                    }
                }
            } catch (IOException e) {
                SwingUtilities.invokeLater(() ->
                        JOptionPane.showMessageDialog(frame, "Connessione fallita: " + e.getMessage()));
            }
        });
        reader.setDaemon(true);
        reader.start();
    }
}