namespace MHTG
{
    partial class AboutForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.AboutPictureBox = new System.Windows.Forms.PictureBox();
            this.AboutTitleLabel = new System.Windows.Forms.Label();
            ((System.ComponentModel.ISupportInitialize)(this.AboutPictureBox)).BeginInit();
            this.SuspendLayout();
            // 
            // AboutPictureBox
            // 
            this.AboutPictureBox.BackgroundImage = global::MHTG.Properties.Resources.green_icon;
            this.AboutPictureBox.BackgroundImageLayout = System.Windows.Forms.ImageLayout.None;
            this.AboutPictureBox.InitialImage = global::MHTG.Properties.Resources.green_icon;
            this.AboutPictureBox.Location = new System.Drawing.Point(5, 5);
            this.AboutPictureBox.Name = "AboutPictureBox";
            this.AboutPictureBox.Size = new System.Drawing.Size(128, 128);
            this.AboutPictureBox.TabIndex = 0;
            this.AboutPictureBox.TabStop = false;
            // 
            // AboutTitleLabel
            // 
            this.AboutTitleLabel.AutoSize = true;
            this.AboutTitleLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            this.AboutTitleLabel.Location = new System.Drawing.Point(139, 9);
            this.AboutTitleLabel.Name = "AboutTitleLabel";
            this.AboutTitleLabel.Size = new System.Drawing.Size(286, 20);
            this.AboutTitleLabel.TabIndex = 1;
            this.AboutTitleLabel.Text = "Modular Hydroponic Tower Garden";
            // 
            // AboutForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(433, 239);
            this.Controls.Add(this.AboutTitleLabel);
            this.Controls.Add(this.AboutPictureBox);
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "AboutForm";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "About";
            this.FormClosed += new System.Windows.Forms.FormClosedEventHandler(this.AboutForm_FormClosed);
            ((System.ComponentModel.ISupportInitialize)(this.AboutPictureBox)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.PictureBox AboutPictureBox;
        private System.Windows.Forms.Label AboutTitleLabel;
    }
}