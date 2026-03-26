package vessel.example;

import javax.swing.*;
import java.awt.*;

public class App {
    public static void main(String[] args) {
        // Run the GUI creation on the Event Dispatch Thread
        SwingUtilities.invokeLater(() -> {
            createAndShowGUI();
        });
        
        System.out.println("Vessel-powered Java GUI started!");
        System.out.println("Running with bundled/shared JRE. 🚢☕️");
    }

    private static void createAndShowGUI() {
        JFrame frame = new JFrame("Vessel Java Example");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.setSize(500, 350);
        frame.setLocationRelativeTo(null); // Center on screen

        // Main layout
        JPanel panel = new JPanel();
        panel.setLayout(new BorderLayout());
        panel.setBackground(new Color(30, 30, 40)); // Dark theme
        panel.setBorder(BorderFactory.createEmptyBorder(20, 20, 20, 20));

        // Title Label
        JLabel titleLabel = new JLabel("🚢 Vessel Java", SwingConstants.CENTER);
        titleLabel.setFont(new Font("SansSerif", Font.BOLD, 28));
        titleLabel.setForeground(new Color(200, 200, 255));
        panel.add(titleLabel, BorderLayout.NORTH);

        // Content Area
        String message = "<html><body style='text-align: center; color: #aaaaaa;'>"
                + "<h2 style='color: white;'>Welcome to the Java Example</h2>"
                + "<p>This application was built with Gradle and bundled by Vessel.</p>"
                + "<p style='margin-top: 10px;'><b>Status:</b> Running on shared JRE in ~/.vessel</p>"
                + "</body></html>";
        
        JLabel contentLabel = new JLabel(message, SwingConstants.CENTER);
        contentLabel.setFont(new Font("SansSerif", Font.PLAIN, 14));
        panel.add(contentLabel, BorderLayout.CENTER);

        // Action Button
        JButton button = new JButton("Ready to Sail!");
        button.setFocusPainted(false);
        button.setBackground(new Color(80, 80, 200));
        button.setForeground(Color.WHITE);
        button.setFont(new Font("SansSerif", Font.BOLD, 14));
        button.addActionListener(e -> {
            JOptionPane.showMessageDialog(frame, "Vessel is a native Linux application bundler.\nIt makes your Java apps truly portable!", "Vessel Info", JOptionPane.INFORMATION_MESSAGE);
        });
        
        JPanel buttonPanel = new JPanel();
        buttonPanel.setOpaque(false);
        buttonPanel.add(button);
        panel.add(buttonPanel, BorderLayout.SOUTH);

        frame.add(panel);
        frame.setVisible(true);
    }
}
