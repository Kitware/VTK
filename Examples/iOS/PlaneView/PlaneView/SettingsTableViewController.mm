/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#import "SettingsTableViewController.h"
#import "VTKViewController.h"

@interface SettingsTableViewController ()

@property (weak, nonatomic) IBOutlet UISwitch *enableprobeSwitch;

@end

@implementation SettingsTableViewController

- (IBAction)EnableProbeChanged:(id)sender
{
  // Inside another ViewController
  VTKViewController *vtkVC = (VTKViewController *)
      [self.navigationController.viewControllers firstObject];
  [vtkVC setProbeEnabled:[sender isOn]];
}

- (void)viewDidLoad
{
  [super viewDidLoad];

  VTKViewController *vtkVC = (VTKViewController *)
      [self.navigationController.viewControllers firstObject];
  [self.enableprobeSwitch setOn:[vtkVC getProbeEnabled]];

  // Uncomment the following line to preserve selection between presentations.
  // self.clearsSelectionOnViewWillAppear = NO;

  // Uncomment the following line to display an Edit button in the navigation bar for this view controller.
  // self.navigationItem.rightBarButtonItem = self.editButtonItem;
}

- (void)didReceiveMemoryWarning
{
  [super didReceiveMemoryWarning];
  // Dispose of any resources that can be recreated.
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  [tableView deselectRowAtIndexPath:indexPath animated:NO];

  if (indexPath.row == 1)
  {
    UIDocumentPickerViewController *documentPicker =
      [[UIDocumentPickerViewController alloc]
        initWithDocumentTypes:@[@"com.kitware.vtu",@"com.kitware.vts",@"com.kitware.vtr",@"com.kitware.vti"]
        inMode:UIDocumentPickerModeImport];
    documentPicker.delegate = self;
    documentPicker.modalPresentationStyle = UIModalPresentationFormSheet;
    [self presentViewController:documentPicker animated:NO completion:nil];

    // UIDocumentMenuViewController *documentPicker =
    //   [[UIDocumentMenuViewController alloc]
    //     initWithDocumentTypes:@[@"com.kitware.vtu",@"com.kitware.vts",@"com.kitware.vtr",@"com.kitware.vti"]
    //     inMode:UIDocumentPickerModeImport];
    // documentPicker.delegate = self;
    // documentPicker.modalPresentationStyle = UIModalPresentationFullScreen;
    // [self presentViewController:documentPicker animated:NO completion:nil];
  }
}

- (void)documentMenu:(UIDocumentMenuViewController *)documentMenu
  didPickDocumentPicker:(UIDocumentPickerViewController *)documentPicker
{
  // documentPicker.delegate = self;
  // documentPicker.modalPresentationStyle = UIModalPresentationFormSheet;
  // [self presentViewController:documentPicker animated:YES completion:nil];
}

- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentAtURL:(NSURL *)url
{
  if (controller.documentPickerMode == UIDocumentPickerModeImport)
  {
    NSString *alertMessage = [NSString stringWithFormat:@"Successfully imported %@", [url lastPathComponent]];
    dispatch_async(dispatch_get_main_queue(), ^{
      UIAlertController *alertController = [UIAlertController
      alertControllerWithTitle:@"Import"
      message:alertMessage
      preferredStyle:UIAlertControllerStyleAlert];
      [alertController addAction:[UIAlertAction actionWithTitle:@"Ok" style:UIAlertActionStyleDefault handler:nil]];
      [self presentViewController:alertController animated:YES completion:nil];
    });

    // pass the data to the mapper
    VTKViewController *vtkVC = (VTKViewController *)
        [self.navigationController.viewControllers firstObject];
    [vtkVC setNewDataFile:url];
  }
}


#pragma mark - Table view data source

/*
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:<#@"reuseIdentifier"#> forIndexPath:indexPath];

    // Configure the cell...

    return cell;
}
*/

/*
// Override to support conditional editing of the table view.
- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
    // Return NO if you do not want the specified item to be editable.
    return YES;
}
*/

/*
// Override to support editing the table view.
- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath {
    if (editingStyle == UITableViewCellEditingStyleDelete) {
        // Delete the row from the data source
        [tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationFade];
    } else if (editingStyle == UITableViewCellEditingStyleInsert) {
        // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
    }
}
*/

/*
// Override to support rearranging the table view.
- (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath {
}
*/

/*
// Override to support conditional rearranging of the table view.
- (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath {
    // Return NO if you do not want the item to be re-orderable.
    return YES;
}
*/

/*
#pragma mark - Navigation

// In a storyboard-based application, you will often want to do a little preparation before navigation
- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender {
    // Get the new view controller using [segue destinationViewController].
    // Pass the selected object to the new view controller.
}
*/

@end
